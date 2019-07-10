// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "nzos/video_proxy/nz_wv_decryptor.h"

#include <list>
#include <vector>

#include "base/command_line.h"
#include "base/logging.h"
#include "base/stl_util.h"
#include "base/strings/string_number_conversions.h"
#include "crypto/encryptor.h"
#include "crypto/symmetric_key.h"
#include "media/base/audio_decoder_config.h"
#include "media/base/cdm_promise.h"
#include "media/base/decoder_buffer.h"
#include "media/base/decrypt_config.h"
#include "media/base/media_switches.h"
#include "media/base/video_decoder_config.h"
#include "media/base/video_frame.h"
#include "media/cdm/json_web_key.h"
#include "nzos/video_proxy/nz_video_proxy_dispatcher.h"
#include "content/public/renderer/render_frame.h"
#include "content/public/renderer/render_view.h"

#include "nzos/video_proxy/nz_video_decoder.h"
#include "nzos/NzApe/QzProperty.h"
#include "nzos/NzApe/NzApe.h"

namespace media {

int NzWvDecryptor::ids = 1;
std::map<int, NzWvDecryptor*> NzWvDecryptor::nz_decryptors_;

const uint8 kWidevineUuid[16] = { 0xED, 0xEF, 0x8B, 0xA9, 0x79, 0xD6, 0x4A,
    0xCE, 0xA3, 0xC8, 0x27, 0xDC, 0xD5, 0x1D, 0x21, 0xED };

const int kBoxHeaderSize = 8;  // Box's header contains Size and Type.
const int kBoxLargeSizeSize = 8;
const int kPsshVersionFlagSize = 4;
const int kPsshSystemIdSize = 16;
const int kPsshDataSizeSize = 4;
const uint32 kTencType = 0x74656e63;
const uint32 kPsshType = 0x70737368;

static uint32 ReadUint32 (const uint8_t* data) {
  uint32 value = 0;
  for (int i = 0; i < 4; ++i)
    value = (value << 8) | data[i];

  return value;
}

static uint64 ReadUint64 (const uint8_t* data) {
  uint64 value = 0;
  for (int i = 0; i < 8; ++i)
    value = (value << 8) | data[i];

  return value;
}

// Tries to find a PSSH box whose "SystemId" is |uuid| in |data|, parses the
// "Data" of the box and put it in |pssh_data|. Returns true if such a box is
// found and successfully parsed. Returns false otherwise.
// Notes:
// 1, If multiple PSSH boxes are found,the "Data" of the first matching PSSH box
// will be set in |pssh_data|.
// 2, Only PSSH and TENC boxes are allowed in |data|. TENC boxes are skipped.
static bool GetPsshData (const std::vector<uint8>& data, const uint8* uuid,
             std::vector<uint8>* pssh_data) {
  const uint8* cur = &data[0];
  const uint8* data_end = &data[data.size () - 1] + 1;
  int bytes_left = data.size ();

  while (bytes_left > 0) {
    const uint8* box_head = cur;

    if (bytes_left < kBoxHeaderSize)
      return false;

    uint64_t box_size = ReadUint32 (cur);
    uint32 type = ReadUint32 (cur + 4);
    cur += kBoxHeaderSize;
    bytes_left -= kBoxHeaderSize;

    if (box_size == 1) {  // LargeSize is present.
      if (bytes_left < kBoxLargeSizeSize)
        return false;

      box_size = ReadUint64 (cur);
      cur += kBoxLargeSizeSize;
      bytes_left -= kBoxLargeSizeSize;
    }
    else if (box_size == 0) {
      box_size = bytes_left + kBoxHeaderSize;
    }

    const uint8* box_end = box_head + box_size;
    if (data_end < box_end)
      return false;

    if (type == kTencType) {
      // Skip 'tenc' box.
      cur = box_end;
      bytes_left = data_end - cur;
      continue;
    }
    else if (type != kPsshType) {
      return false;
    }

    const int kPsshBoxMinimumSize = kPsshVersionFlagSize + kPsshSystemIdSize
        + kPsshDataSizeSize;
    if (box_end < cur + kPsshBoxMinimumSize)
      return false;

    uint32 version_and_flags = ReadUint32 (cur);
    cur += kPsshVersionFlagSize;
    bytes_left -= kPsshVersionFlagSize;
    if (version_and_flags != 0)
      return false;

    DCHECK_GE(bytes_left, kPsshSystemIdSize);
    if (memcmp (uuid, cur, 16) != 0) {
      cur = box_end;
      bytes_left = data_end - cur;
      continue;
    }

    cur += kPsshSystemIdSize;
    bytes_left -= kPsshSystemIdSize;

    uint32 data_size = ReadUint32 (cur);
    cur += kPsshDataSizeSize;
    bytes_left -= kPsshDataSizeSize;

    if (box_end < cur + data_size)
      return false;

    pssh_data->assign (cur, cur + data_size);
    return true;
  }

  return false;
}

NzWvDecryptor* NzWvDecryptor::getNzWvDecryptor (int id) {

  std::map<int, NzWvDecryptor*>::iterator it = nz_decryptors_.find (id);

  if (it != nz_decryptors_.end ()) {
    return it->second;
  }

  return NULL;
}

// Keeps track of the session IDs and DecryptionKeys. The keys are ordered by
// insertion time (last insertion is first). It takes ownership of the
// DecryptionKeys.
class NzWvDecryptor::SessionIdDecryptionKeyMap {
  // Use a std::list to actually hold the data. Insertion is always done
  // at the front, so the "latest" decryption key is always the first one
  // in the list.
  typedef std::list<std::pair<std::string, DecryptionKey*> > KeyList;

public:
  SessionIdDecryptionKeyMap () {
  }
  ~SessionIdDecryptionKeyMap () {
    STLDeleteValues (&key_list_);
  }

  // Replaces value if |session_id| is already present, or adds it if not.
  // This |decryption_key| becomes the latest until another insertion or
  // |session_id| is erased.
  void Insert (const std::string& web_session_id,
          scoped_ptr<DecryptionKey> decryption_key);

  // Deletes the entry for |session_id| if present.
  void Erase (const std::string& web_session_id);

  // Returns whether the list is empty
  bool Empty () const {
    return key_list_.empty ();
  }

  // Returns the last inserted DecryptionKey.
  DecryptionKey* LatestDecryptionKey () {
    DCHECK(!key_list_.empty ());
    return key_list_.begin ()->second;
  }

  bool Contains (const std::string& web_session_id) {
    return Find (web_session_id) != key_list_.end ();
  }

  std::string LatestSessionId () {
    DCHECK(!key_list_.empty ());
    return key_list_.begin ()->first;
  }

private:
  // Searches the list for an element with |web_session_id|.
  KeyList::iterator Find (const std::string& web_session_id);

  // Deletes the entry pointed to by |position|.
  void Erase (KeyList::iterator position);

  KeyList key_list_;

  DISALLOW_COPY_AND_ASSIGN(SessionIdDecryptionKeyMap);
};

void NzWvDecryptor::SessionIdDecryptionKeyMap::Insert (
    const std::string& web_session_id,
    scoped_ptr<DecryptionKey> decryption_key) {
  KeyList::iterator it = Find (web_session_id);
  if (it != key_list_.end ())
    Erase (it);
  DecryptionKey* raw_ptr = decryption_key.release ();
  key_list_.push_front (std::make_pair (web_session_id, raw_ptr));
}

void NzWvDecryptor::SessionIdDecryptionKeyMap::Erase (
    const std::string& web_session_id) {
  KeyList::iterator it = Find (web_session_id);
  if (it == key_list_.end ())
    return;
  Erase (it);
}

NzWvDecryptor::SessionIdDecryptionKeyMap::KeyList::iterator
NzWvDecryptor::SessionIdDecryptionKeyMap::Find (
    const std::string& web_session_id) {
  for (KeyList::iterator it = key_list_.begin (); it != key_list_.end ();
      ++it) {
    if (it->first == web_session_id)
      return it;
  }
  return key_list_.end ();
}

void NzWvDecryptor::SessionIdDecryptionKeyMap::Erase (KeyList::iterator position) {
  DCHECK(position->second);
  delete position->second;
  key_list_.erase (position);
}

uint32 NzWvDecryptor::next_web_session_id_ = 1;

enum ClearBytesBufferSel {
  kSrcContainsClearBytes,
  kDstContainsClearBytes
};

// Decrypts |input| using |key|.  Returns a DecoderBuffer with the decrypted
// data if decryption succeeded or NULL if decryption failed.
// This is for Nz where decryption is bypassed
static scoped_refptr<DecoderBuffer> DecryptDataNz (const DecoderBuffer& input,
                                                   uint32_t sessionId) {
  CHECK(input.data_size ());
  CHECK(input.decrypt_config ());

  const char* sample = reinterpret_cast<const char*> (input.data ());
  size_t sample_size = static_cast<size_t> (input.data_size ());

  DCHECK_GT(sample_size, 0U) << "No sample data to be decrypted.";
  if (sample_size == 0)
    return NULL;

  // This is the code that forwards the buffer encrypted.
  scoped_refptr<DecoderBuffer> output = DecoderBuffer::CopyFrom (
      reinterpret_cast<const uint8*> (sample), sample_size);
  output->set_decrypt_config (
      make_scoped_ptr (
          new DecryptConfig (input.decrypt_config ()->key_id (),
                             input.decrypt_config ()->iv (),
                             input.decrypt_config ()->subsamples (),
							 sessionId)));
  return output;
}

NzWvDecryptor::NzWvDecryptor (
    const SessionMessageCB& session_message_cb,
    const SessionClosedCB& session_closed_cb,
    const SessionKeysChangeCB& session_keys_change_cb) :
    session_message_cb_ (session_message_cb),
    session_closed_cb_ (session_closed_cb),
    session_keys_change_cb_ (session_keys_change_cb) {
  DCHECK(!session_message_cb_.is_null ());
  DCHECK(!session_closed_cb_.is_null ());
  DCHECK(!session_keys_change_cb_.is_null());

  baseSessionId = ((content::NzVideoProxyDispatcher::Instance ()->RenderId ())
      * 15000);
  // Id used in Browser - this calculation may not be needed as there is a separate
  // proxy handler for each renderer.
  id_ = baseSessionId + ids++;

  LOG(INFO) << "NzWvDecryptor Construct: " << id_;

  Nz_Decrypt_Create create_data;
  create_data.id = id_;
  create_data.scheme = e_QzPropertyDrmScheme_Widevine;

  content::NzVideoProxyDispatcher::Instance ()->CreateDecryptor (create_data);

  nz_decryptors_[id_] = this;

}

NzWvDecryptor::~NzWvDecryptor () {
  key_map_.clear ();
  LOG(INFO) << "NzWvDecryptor Destruct: " << id_;

  Nz_Session_Release session_data;
  session_data.id = id_;
  content::NzVideoProxyDispatcher::Instance ()->ReleaseSession (session_data);

  for (std::set<std::string>::iterator it = valid_sessions_.begin ();
      it != valid_sessions_.end (); it++) {

    int session_id;
    base::StringToInt (*it, &session_id);
    session_id += baseSessionId;

    LOG(INFO) << "ReleaseSession(~): " << session_id;
  }
}

bool NzWvDecryptor::NzWvCapable () {
  return content::NzVideoProxyDispatcher::Instance ()->WidevineCapable ();
}

void NzWvDecryptor::SetServerCertificate (const std::vector<uint8_t>& certificate,
                                     scoped_ptr<SimpleCdmPromise> promise) {
  promise->reject (NOT_SUPPORTED_ERROR, 0,
                   "SetServerCertificate() is not supported.");
}

void NzWvDecryptor::CreateSessionAndGenerateRequest (
    SessionType session_type,
    EmeInitDataType init_data_type,
    const std::vector<uint8_t>& init_data,
    scoped_ptr<NewSessionCdmPromise> promise) {

  std::string web_session_id (base::UintToString (next_web_session_id_++));
  valid_sessions_.insert (web_session_id);

  int session_id = baseSessionId + next_web_session_id_ - 1;

  uint32_t promise_id = cdm_promise_adapter_.SavePromise (promise.Pass ());

  promises_[session_id] = promise_id;

  Nz_Generate_Key_Request request_data;
  request_data.id = id_;
  request_data.key_rqst_id = session_id;
  request_data.scheme = e_QzPropertyDrmScheme_Widevine;
  if (init_data.size ()) {
    std::vector<uint8> pssh_data;
    if (!GetPsshData (init_data, &kWidevineUuid[0], &pssh_data)) {
      promise->reject (INVALID_ACCESS_ERROR, 0,
                       "Widevine init data not found.");
      return;
    }
    request_data.init_data.assign (pssh_data.begin (), pssh_data.end ());
  }

  LOG(INFO) << "CreateSession: " << session_id;
  content::NzVideoProxyDispatcher::Instance ()->GenerateKeyRequest (
      request_data);

}

void NzWvDecryptor::KeyRequest (uint32_t sessionId, uint32_t keyRqstId,
                                std::vector<uint8> opaque_data, std::string url) {

  PromiseMap::iterator it = promises_.find (keyRqstId);
  if (it == promises_.end ()) {
    LOG(ERROR) << "Cannot find session for : " << keyRqstId;
    return;
  }

  LOG(INFO) << "Key Request received for: " << keyRqstId
               << " Opaque Data Size " << opaque_data.size ();

  uint32_t loc_session_id = keyRqstId % baseSessionId;

  char web_session_id[20];
  sprintf (web_session_id, "%d", loc_session_id);
  std::string sid = web_session_id;

  cdm_promise_adapter_.ResolvePromise (promises_[keyRqstId], sid);

  session_message_cb_.Run (web_session_id,
                           MediaKeys::LICENSE_REQUEST,
                           opaque_data,
                           GURL::EmptyGURL());
}

void NzWvDecryptor::LoadSession (SessionType session_type,
                                 const std::string& web_session_id,
                                 scoped_ptr<NewSessionCdmPromise> promise) {
  // TODO(xhwang): Change this to NOTREACHED() when blink checks for key systems
  // that do not support loadSession. See http://crbug.com/342481
  promise->reject (NOT_SUPPORTED_ERROR, 0, "LoadSession() is not supported.");
}

void NzWvDecryptor::UpdateSession (const std::string& session_id,
                                   const std::vector<uint8_t>& response,
                                   scoped_ptr<SimpleCdmPromise> promise) {
  CHECK(!response.empty ());

  // TODO(jrummell): Convert back to a DCHECK once prefixed EME is removed.
  if (valid_sessions_.find(session_id) == valid_sessions_.end()) {
    promise->reject(INVALID_ACCESS_ERROR, 0, "Session does not exist.");
    return;
  }

  Nz_Key_Data key_data;
  key_data.id = id_;

  int sess_id;
  base::StringToInt (session_id, &sess_id);
  sess_id += baseSessionId;

  key_data.key_rqst_id = sess_id;

  //key_data.init_data.insert(key_data.init_data.begin(),
  //	it->first.c_str(), it->first.c_str() + it->first.length());
  key_data.key_data.assign(response.begin(), response.end());

  LOG(INFO) << "UpdateSession: " << session_id;
  content::NzVideoProxyDispatcher::Instance ()->UpdateSession (key_data);

  {
    base::AutoLock auto_lock (new_key_cb_lock_);

    if (!new_audio_key_cb_.is_null ())
      new_audio_key_cb_.Run ();

    if (!new_video_key_cb_.is_null ())
      new_video_key_cb_.Run ();
  }

  promise->resolve ();

  // TODO:
  //session_keys_change_cb_.Run(session_id, key_added, keys_info.Pass());

}

void NzWvDecryptor::CloseSession (const std::string& web_session_id,
                                  scoped_ptr<SimpleCdmPromise> promise) {
  // Validate that this is a reference to an active session and then forget it.
  std::set<std::string>::iterator it = valid_sessions_.find (web_session_id);
  // TODO(jrummell): Convert back to a DCHECK once prefixed EME is removed.
  if (it == valid_sessions_.end ()) {
    promise->reject (INVALID_ACCESS_ERROR, 0, "Session does not exist.");
    return;
  }

  valid_sessions_.erase (it);

  int sessionId;
  base::StringToInt (web_session_id, &sessionId);
  sessionId += baseSessionId;

  LOG(INFO) << "ReleaseSession: " << sessionId;

  // Close the session.
  DeleteKeysForSession (web_session_id);
  promise->resolve ();
  session_closed_cb_.Run (web_session_id);
}

void NzWvDecryptor::RemoveSession (const std::string& web_session_id,
                                   scoped_ptr<SimpleCdmPromise> promise) {
  // NzWvDecryptor doesn't keep any persistent data, so this should be
  // NOT_REACHED().
  // TODO(jrummell): Make sure persistent session types are rejected.
  // http://crbug.com/384152.
  //
  // However, v0.1b calls to CancelKeyRequest() will call this, so close the
  // session, if it exists.
  // TODO(jrummell): Remove the close() call when prefixed EME is removed.
  // http://crbug.com/249976.
  if (valid_sessions_.find (web_session_id) != valid_sessions_.end ()) {
    CloseSession (web_session_id, promise.Pass ());
    return;
  }

  promise->reject (INVALID_ACCESS_ERROR, 0, "Session does not exist.");
}

CdmContext* NzWvDecryptor::GetCdmContext () {
  return this;
}

Decryptor* NzWvDecryptor::GetDecryptor () {
  return this;
}

int NzWvDecryptor::GetCdmId() const {
  return kInvalidCdmId;
}

void NzWvDecryptor::RegisterNewKeyCB (StreamType stream_type,
                                      const NewKeyCB& new_key_cb) {
  base::AutoLock auto_lock (new_key_cb_lock_);

  switch (stream_type) {
    case kAudio:
      new_audio_key_cb_ = new_key_cb;
      break;
    case kVideo:
      new_video_key_cb_ = new_key_cb;
      break;
    default:
      NOTREACHED();
  }
}

void NzWvDecryptor::Decrypt (StreamType stream_type,
                             const scoped_refptr<DecoderBuffer>& encrypted,
                             const DecryptCB& decrypt_cb) {
  CHECK(encrypted->decrypt_config());

  scoped_refptr<DecoderBuffer> decrypted;

  // An empty iv string signals that the frame is unencrypted.
  if (encrypted->decrypt_config()->iv ().empty()) {
    decrypted = DecoderBuffer::CopyFrom (encrypted->data (),
                                         encrypted->data_size ());
  }
  else {

    decrypted = DecryptDataNz (*encrypted.get (), id_);
    if (!decrypted.get ()) {
      DVLOG(1) << "Decryption failed.";
      decrypt_cb.Run (kError, NULL);
      return;
    }
  }

  decrypted->set_timestamp (encrypted->timestamp ());
  decrypted->set_duration (encrypted->duration ());

  decrypt_cb.Run (kSuccess, decrypted);
}

void NzWvDecryptor::CancelDecrypt (StreamType stream_type) {
  // Decrypt() calls the DecryptCB synchronously so there's nothing to cancel.
}

void NzWvDecryptor::InitializeAudioDecoder (const AudioDecoderConfig& config,
                                            const DecoderInitCB& init_cb) {
  // NzWvDecryptor does not support audio decoding.
  init_cb.Run (false);
}

void NzWvDecryptor::InitializeVideoDecoder (const VideoDecoderConfig& config,
                                            const DecoderInitCB& init_cb) {
  // NzWvDecryptor does not support video decoding.
  init_cb.Run (false);
}

void NzWvDecryptor::DecryptAndDecodeAudio (
    const scoped_refptr<DecoderBuffer>& encrypted,
    const AudioDecodeCB& audio_decode_cb) {
  NOTREACHED() << "NzWvDecryptor does not support audio decoding";
}

void NzWvDecryptor::DecryptAndDecodeVideo (
    const scoped_refptr<DecoderBuffer>& encrypted,
    const VideoDecodeCB& video_decode_cb) {
  NOTREACHED() << "NzWvDecryptor does not support video decoding";
}

void NzWvDecryptor::ResetDecoder (StreamType stream_type) {
  NOTREACHED() << "NzWvDecryptor does not support audio/video decoding";
}

void NzWvDecryptor::DeinitializeDecoder (StreamType stream_type) {
  NOTREACHED() << "NzWvDecryptor does not support audio/video decoding";
}

uint32_t NzWvDecryptor::GetDrmScheme () {
  return e_QzPropertyDrmScheme_Widevine;
}

bool NzWvDecryptor::AddDecryptionKey (const std::string& web_session_id,
                                      const std::string& key_id,
                                      const std::string& key_string) {
  scoped_ptr<DecryptionKey> decryption_key (new DecryptionKey (key_string));
  if (!decryption_key->Init ()) {
    DVLOG(1) << "Could not initialize decryption key.";
    return false;
  }

  base::AutoLock auto_lock (key_map_lock_);
  KeyIdToSessionKeysMap::iterator key_id_entry = key_map_.find (key_id);
  if (key_id_entry != key_map_.end ()) {
    key_id_entry->second->Insert (web_session_id, decryption_key.Pass ());
    return true;
  }

  // |key_id| not found, so need to create new entry.
  scoped_ptr<SessionIdDecryptionKeyMap> inner_map (
      new SessionIdDecryptionKeyMap ());
  inner_map->Insert (web_session_id, decryption_key.Pass ());
  key_map_.add (key_id, inner_map.Pass ());
  return true;
}

NzWvDecryptor::DecryptionKey* NzWvDecryptor::GetKey_Locked (
    const std::string& key_id) const {
  base::AutoLock auto_lock (key_map_lock_);
  KeyIdToSessionKeysMap::const_iterator key_id_found = key_map_.find (key_id);
  if (key_id_found == key_map_.end ())
    return NULL;

  // Return the key from the "latest" session_id entry.
  return key_id_found->second->LatestDecryptionKey ();
}

void NzWvDecryptor::DeleteKeysForSession (const std::string& web_session_id) {
  base::AutoLock auto_lock (key_map_lock_);

  // Remove all keys associated with |web_session_id|. Since the data is
  // optimized for access in GetKey(), we need to look at each entry in
  // |key_map_|.
  KeyIdToSessionKeysMap::iterator it = key_map_.begin ();
  while (it != key_map_.end ()) {
    it->second->Erase (web_session_id);
    if (it->second->Empty ()) {
      // Need to get rid of the entry for this key_id. This will mess up the
      // iterator, so we need to increment it first.
      KeyIdToSessionKeysMap::iterator current = it;
      ++it;
      key_map_.erase (current);
    }
    else {
      ++it;
    }
  }
}

NzWvDecryptor::DecryptionKey::DecryptionKey (const std::string& secret) :
    secret_ (secret) {
}

NzWvDecryptor::DecryptionKey::~DecryptionKey () {
}

bool NzWvDecryptor::DecryptionKey::Init () {
  CHECK(!secret_.empty ());
  decryption_key_.reset (
      crypto::SymmetricKey::Import (crypto::SymmetricKey::AES, secret_));
  if (!decryption_key_)
    return false;
  return true;
}

}  // namespace media
