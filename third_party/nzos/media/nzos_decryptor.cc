// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "third_party/nzos/media/nzos_decryptor.h"

#include <stddef.h>
#include <list>
#include <memory>
#include <utility>
#include <vector>

#include "base/logging.h"
#include "base/macros.h"
#include "base/strings/string_number_conversions.h"
#include "base/time/time.h"
#include "crypto/symmetric_key.h"
#include "media/base/audio_decoder_config.h"
#include "media/base/callback_registry.h"
#include "media/base/cdm_promise.h"
#include "media/base/decoder_buffer.h"
#include "media/base/decrypt_config.h"
#include "media/base/limits.h"
#include "media/base/video_decoder_config.h"
#include "media/base/video_frame.h"
#include "media/cdm/cbcs_decryptor.h"
#include "media/cdm/cenc_decryptor.h"
#include "media/cdm/cenc_utils.h"
#include "media/cdm/json_web_key.h"

#include "third_party/nzos/include/NzApe.h"
#include "third_party/nzos/include/QzProperty.h"

namespace media {

const uint8_t kWidevineUuid[16] = {0xED, 0xEF, 0x8B, 0xA9, 0x79, 0xD6,
                                   0x4A, 0xCE, 0xA3, 0xC8, 0x27, 0xDC,
                                   0xD5, 0x1D, 0x21, 0xED};

int NzosDecryptor::nextKeyRequestId = 1;
int NzosDecryptor::nextSessionId = 1;

// This id is not used anymore.
int NzosDecryptor::ids = 1;
std::map<int, NzosDecryptor*> NzosDecryptor::nz_decryptors_;
NzosMediaProxyInterface* NzosDecryptor::proxyInterface;

NzosDecryptor* NzosDecryptor::getDecryptor(int id) {
  std::map<int, NzosDecryptor*>::iterator it = nz_decryptors_.find(id);

  if (it != nz_decryptors_.end()) {
    return it->second;
  }

  return NULL;
}

const int kBoxHeaderSize = 8;  // Box's header contains Size and Type.
const int kBoxLargeSizeSize = 8;
const int kPsshVersionFlagSize = 4;
const int kPsshSystemIdSize = 16;
const int kPsshDataSizeSize = 4;
const uint32_t kTencType = 0x74656e63;
const uint32_t kPsshType = 0x70737368;

static uint32_t ReadUint32(const uint8_t* data) {
  uint32_t value = 0;
  for (int i = 0; i < 4; ++i)
    value = (value << 8) | data[i];

  return value;
}

static uint64_t ReadUint64(const uint8_t* data) {
  uint64_t value = 0;
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
static bool GetPsshData(const std::vector<uint8_t>& data,
                        const uint8_t* uuid,
                        std::vector<uint8_t>* pssh_data) {
  const uint8_t* cur = &data[0];
  const uint8_t* data_end = &data[data.size() - 1] + 1;
  int bytes_left = data.size();

  while (bytes_left > 0) {
    const uint8_t* box_head = cur;

    if (bytes_left < kBoxHeaderSize)
      return false;

    uint64_t box_size = ReadUint32(cur);
    uint32_t type = ReadUint32(cur + 4);
    cur += kBoxHeaderSize;
    bytes_left -= kBoxHeaderSize;

    if (box_size == 1) {  // LargeSize is present.
      if (bytes_left < kBoxLargeSizeSize)
        return false;

      box_size = ReadUint64(cur);
      cur += kBoxLargeSizeSize;
      bytes_left -= kBoxLargeSizeSize;
    } else if (box_size == 0) {
      box_size = bytes_left + kBoxHeaderSize;
    }

    const uint8_t* box_end = box_head + box_size;
    if (data_end < box_end)
      return false;

    if (type == kTencType) {
      // Skip 'tenc' box.
      cur = box_end;
      bytes_left = data_end - cur;
      continue;
    } else if (type != kPsshType) {
      return false;
    }

    const int kPsshBoxMinimumSize =
        kPsshVersionFlagSize + kPsshSystemIdSize + kPsshDataSizeSize;
    if (box_end < cur + kPsshBoxMinimumSize)
      return false;

    uint32_t version_and_flags = ReadUint32(cur);
    cur += kPsshVersionFlagSize;
    bytes_left -= kPsshVersionFlagSize;
    if (version_and_flags != 0)
      return false;

    DCHECK_GE(bytes_left, kPsshSystemIdSize);
    if (memcmp(uuid, cur, 16) != 0) {
      cur = box_end;
      bytes_left = data_end - cur;
      continue;
    }

    cur += kPsshSystemIdSize;
    bytes_left -= kPsshSystemIdSize;

    uint32_t data_size = ReadUint32(cur);
    cur += kPsshDataSizeSize;
    bytes_left -= kPsshDataSizeSize;

    if (box_end < cur + data_size)
      return false;

    pssh_data->assign(cur, cur + data_size);
    return true;
  }

  return false;
}

namespace {

// Vastly simplified ACM random class, based on media/base/test_random.h.
// base/rand_util.h doesn't work in the sandbox. This class generates
// predictable sequences of pseudorandom numbers. These are only used for
// persistent session IDs, so unpredictable sequences are not necessary.
uint32_t Rand(uint32_t seed) {
  static const uint64_t A = 16807;        // bits 14, 8, 7, 5, 2, 1, 0
  static const uint64_t M = 2147483647L;  // 2^32-1
  return static_cast<uint32_t>((seed * A) % M);
}

// Create a random session ID. Returned value is a printable string to make
// logging the session ID easier.
std::string GenerateSessionId(int& sessionId_int) {
  // Create a random value. There is a slight chance that the same ID is
  // generated in different processes, but session IDs are only ever saved
  // by External Clear Key, which is test only.
  static uint32_t seed = 0;
  if (!seed) {
    // If this is the first call, use the current time as the starting value.
    seed = static_cast<uint32_t>(base::Time::Now().ToInternalValue());
  }
  seed = Rand(seed);

  // Include an incrementing value to ensure that the session ID is unique
  // in this process.
  static int next_session_id_suffix = 0;
  next_session_id_suffix++;

  // return base::HexEncode(&seed, sizeof(seed)) +
  //        base::HexEncode(&next_session_id_suffix,
  //                        sizeof(next_session_id_suffix));
  sessionId_int = seed + next_session_id_suffix;
  return base::HexEncode(&sessionId_int, sizeof(sessionId_int));
}

}  // namespace

// static
void NzosDecryptor::SetProxyInterface(NzosMediaProxyInterface* inst) {
  LOG(ERROR) << "SetProxyInterface";
  proxyInterface = inst;
}

// Keeps track of the session IDs and DecryptionKeys. The keys are ordered by
// insertion time (last insertion is first). It takes ownership of the
// DecryptionKeys.
class NzosDecryptor::SessionIdDecryptionKeyMap {
  // Use a std::list to actually hold the data. Insertion is always done
  // at the front, so the "latest" decryption key is always the first one
  // in the list.
  using KeyList =
      std::list<std::pair<std::string, std::unique_ptr<DecryptionKey>>>;

 public:
  SessionIdDecryptionKeyMap() = default;
  ~SessionIdDecryptionKeyMap() = default;

  // Replaces value if |session_id| is already present, or adds it if not.
  // This |decryption_key| becomes the latest until another insertion or
  // |session_id| is erased.
  void Insert(const std::string& session_id,
              std::unique_ptr<DecryptionKey> decryption_key);

  // Deletes the entry for |session_id| if present.
  void Erase(const std::string& session_id);

  // Returns whether the list is empty
  bool Empty() const { return key_list_.empty(); }

  // Returns the last inserted DecryptionKey.
  DecryptionKey* LatestDecryptionKey() {
    DCHECK(!key_list_.empty());
    return key_list_.begin()->second.get();
  }

  bool Contains(const std::string& session_id) {
    return Find(session_id) != key_list_.end();
  }

 private:
  // Searches the list for an element with |session_id|.
  KeyList::iterator Find(const std::string& session_id);

  // Deletes the entry pointed to by |position|.
  void Erase(KeyList::iterator position);

  KeyList key_list_;

  DISALLOW_COPY_AND_ASSIGN(SessionIdDecryptionKeyMap);
};

void NzosDecryptor::SessionIdDecryptionKeyMap::Insert(
    const std::string& session_id,
    std::unique_ptr<DecryptionKey> decryption_key) {
  auto it = Find(session_id);
  if (it != key_list_.end())
    Erase(it);
  key_list_.push_front(std::make_pair(session_id, std::move(decryption_key)));
}

void NzosDecryptor::SessionIdDecryptionKeyMap::Erase(
    const std::string& session_id) {
  auto it = Find(session_id);
  if (it == key_list_.end())
    return;
  Erase(it);
}

NzosDecryptor::SessionIdDecryptionKeyMap::KeyList::iterator
NzosDecryptor::SessionIdDecryptionKeyMap::Find(const std::string& session_id) {
  for (auto it = key_list_.begin(); it != key_list_.end(); ++it) {
    if (it->first == session_id)
      return it;
  }
  return key_list_.end();
}

void NzosDecryptor::SessionIdDecryptionKeyMap::Erase(
    KeyList::iterator position) {
  DCHECK(position->second);
  key_list_.erase(position);
}

// enum ClearBytesBufferSel {
//   kSrcContainsClearBytes,
//   kDstContainsClearBytes
// };

/*
static void CopySubsamples (const std::vector<SubsampleEntry>& subsamples,
                            const ClearBytesBufferSel sel,
                            const uint8_t* src,
                            uint8_t* dst) {
  for (size_t i = 0; i < subsamples.size (); i++) {
    const SubsampleEntry& subsample = subsamples[i];
    if (sel == kSrcContainsClearBytes) {
      src += subsample.clear_bytes;
    } else {
      dst += subsample.clear_bytes;
    }
    memcpy (dst, src, subsample.cypher_bytes);
    src += subsample.cypher_bytes;
    dst += subsample.cypher_bytes;
  }
}

// Decrypts |input| using |key|.  Returns a DecoderBuffer with the decrypted
// data if decryption succeeded or NULL if decryption failed.
static scoped_refptr<DecoderBuffer>
DecryptData (const DecoderBuffer& input, crypto::SymmetricKey* key) {
  CHECK(input.data_size ());
  CHECK(input.decrypt_config ());
  CHECK(key);

  crypto::Encryptor encryptor;
  if (!encryptor.Init (key, crypto::Encryptor::CTR, "")) {
    DVLOG(1) << "Could not initialize decryptor.";
    return NULL;
  }

  DCHECK_EQ(input.decrypt_config ()->iv ().size (),
            static_cast<size_t> (DecryptConfig::kDecryptionKeySize));
  if (!encryptor.SetCounter (input.decrypt_config ()->iv ())) {
    DVLOG(1) << "Could not set counter block.";
    return NULL;
  }

  const char* sample = reinterpret_cast<const char*> (input.data ());
  size_t sample_size = static_cast<size_t> (input.data_size ());

  DCHECK_GT(sample_size, 0U) << "No sample data to be decrypted.";
  if (sample_size == 0)
    return NULL;

  if (input.decrypt_config ()->subsamples ().empty ()) {
    std::string decrypted_text;
    base::StringPiece encrypted_text (sample, sample_size);
    if (!encryptor.Decrypt (encrypted_text, &decrypted_text)) {
      DVLOG(1) << "Could not decrypt data.";
      return NULL;
    }

    // TODO(xhwang): Find a way to avoid this data copy.
    return DecoderBuffer::CopyFrom (
        reinterpret_cast<const uint8_t*> (decrypted_text.data ()),
        decrypted_text.size ());
  }

  const std::vector<SubsampleEntry>& subsamples =
      input.decrypt_config ()->subsamples ();

  size_t total_clear_size = 0;
  size_t total_encrypted_size = 0;
  for (size_t i = 0; i < subsamples.size (); i++) {
    total_clear_size += subsamples[i].clear_bytes;
    total_encrypted_size += subsamples[i].cypher_bytes;
    // Check for overflow. This check is valid because *_size is unsigned.
    DCHECK(total_clear_size >= subsamples[i].clear_bytes);
    if (total_encrypted_size < subsamples[i].cypher_bytes)
      return NULL;
  }
  size_t total_size = total_clear_size + total_encrypted_size;
  if (total_size < total_clear_size || total_size != sample_size) {
    DVLOG(1) << "Subsample sizes do not equal input size";
    return NULL;
  }

  // No need to decrypt if there is no encrypted data.
  if (total_encrypted_size <= 0) {
    return DecoderBuffer::CopyFrom (reinterpret_cast<const uint8_t*> (sample),
                                    sample_size);
  }

  // The encrypted portions of all subsamples must form a contiguous block,
  // such that an encrypted subsample that ends away from a block boundary is
  // immediately followed by the start of the next encrypted subsample. We
  // copy all encrypted subsamples to a contiguous buffer, decrypt them, then
  // copy the decrypted bytes over the encrypted bytes in the output.
  // TODO(strobe): attempt to reduce number of memory copies
  scoped_ptr<uint8_t[]> encrypted_bytes (new uint8_t[total_encrypted_size]);
  CopySubsamples (subsamples, kSrcContainsClearBytes,
                  reinterpret_cast<const uint8_t*> (sample),
                  encrypted_bytes.get ());

  base::StringPiece encrypted_text (
      reinterpret_cast<const char*> (encrypted_bytes.get ()),
      total_encrypted_size);
  std::string decrypted_text;
  if (!encryptor.Decrypt (encrypted_text, &decrypted_text)) {
    DVLOG(1) << "Could not decrypt data.";
    return NULL;
  }
  DCHECK_EQ(decrypted_text.size (), encrypted_text.size ());

  scoped_refptr<DecoderBuffer> output = DecoderBuffer::CopyFrom (
      reinterpret_cast<const uint8_t*> (sample), sample_size);
  CopySubsamples (subsamples, kDstContainsClearBytes,
                  reinterpret_cast<const uint8_t*> (decrypted_text.data ()),
                  output->writable_data ());
  return output;
}
*/

// This code doesn't do any decrypting, data is passed encrypted for ultimate
//
static scoped_refptr<DecoderBuffer> DecryptDataNz(const DecoderBuffer& input,
                                                  uint32_t sessionId,
                                                  uint32_t scheme) {
  CHECK(input.data_size());
  CHECK(input.decrypt_config());

  const char* sample = reinterpret_cast<const char*>(input.data());
  size_t sample_size = static_cast<size_t>(input.data_size());

  DCHECK_GT(sample_size, 0U) << "No sample data to be decrypted.";
  if (sample_size == 0)
    return nullptr;

  // This is the code that forwards the buffer encrypted.
  // That is no decryption is done here
  scoped_refptr<DecoderBuffer> output = DecoderBuffer::CopyFrom(
      reinterpret_cast<const uint8_t*>(sample), sample_size);

  output->set_decrypt_config(media::DecryptConfig::CreateCencConfig(
      input.decrypt_config()->key_id(), input.decrypt_config()->iv(),
      input.decrypt_config()->subsamples(), sessionId, scheme));
  return output;
}

NzosDecryptor::NzosDecryptor(
    const SessionMessageCB& session_message_cb,
    const SessionClosedCB& session_closed_cb,
    const SessionKeysChangeCB& session_keys_change_cb,
    const SessionExpirationUpdateCB& session_expiration_update_cb,
    const std::string& key_system)
    : session_message_cb_(session_message_cb),
      session_closed_cb_(session_closed_cb),
      session_keys_change_cb_(session_keys_change_cb),
      session_expiration_update_cb_(session_expiration_update_cb),
      scheme_(e_QzPropertyDrmScheme_ClearKey) {
  DCHECK(!session_message_cb_.is_null());
  DCHECK(!session_closed_cb_.is_null());
  DCHECK(!session_keys_change_cb_.is_null());
  DCHECK(!session_expiration_update_cb.is_null());

  LOG(INFO) << "NzosDecryptor Construct: ";

  sId_ = GenerateSessionId(id_);

  if (key_system.compare("com.widevine.alpha") == 0) {
    scheme_ = e_QzPropertyDrmScheme_Widevine;
  }

}

NzosDecryptor::~NzosDecryptor() {
  key_map_.clear();
  LOG(INFO) << "NzosDecryptor Destruct: " << id_;

  Nz_Session_Release session_data;
  session_data.id = id_;
  if (proxyInterface) {
    proxyInterface->ReleaseSession(session_data);
  }
}

bool NzosDecryptor::NzosAesCapable() {
  // TODOSJ: Hook this in to Mojo
  return false;
  // TODOSJ
  // return content::NzVideoProxyDispatcher::Instance ()->ClearkeyCapable ();
}

void NzosDecryptor::SetServerCertificate(
    const std::vector<uint8_t>& certificate,
    std::unique_ptr<SimpleCdmPromise> promise) {
  promise->reject(CdmPromise::Exception::NOT_SUPPORTED_ERROR, 0,
                  "SetServerCertificate() is not supported.");
}

void NzosDecryptor::CreateSessionAndGenerateRequest(
    CdmSessionType session_type,
    EmeInitDataType init_data_type,
    const std::vector<uint8_t>& init_data,
    std::unique_ptr<NewSessionCdmPromise> promise) {
  // TODO: Issues with InstanceId not set yet?
  uint32_t session_id = id_ + nextSessionId++;

  nz_decryptors_[session_id] = this;
  SendCreate(session_id);

  // std::string session_id_str = GenerateSessionId(session_id);
  std::string session_id_str = base::NumberToString(session_id);

  CreateSession(session_id_str, session_type);

  LOG(INFO) << "CreateSessionAndGenerateRequest: init_data.size() "
            << init_data.size() << ", init data type: " << (int)init_data_type;

  // TODO(sjames) validate init_data_type - webm - may not be supported in
  // client
  std::vector<uint8_t> message;
  Nz_Generate_Key_Request request_data;

  if (scheme_ == e_QzPropertyDrmScheme_ClearKey) {
    if (!init_data.empty()) {
      std::vector<std::vector<uint8_t>> keys;
      switch (init_data_type) {
        case EmeInitDataType::WEBM:
          // |init_data| is simply the key needed.
          keys.push_back(init_data);
          // TODO:
          break;
        case EmeInitDataType::CENC:
          // |init_data| is a set of 0 or more concatenated 'pssh' boxes.
          if (!GetKeyIdsForCommonSystemId(init_data, &keys)) {
            promise->reject(CdmPromise::Exception::NOT_SUPPORTED_ERROR, 0,
                            "No supported PSSH box found.");
            LOG(ERROR) << "No supported PSSH box found";
            return;
          }
          LOG(INFO) << "CreateSessionAndGenerateRequest: keys.size() "
                    << keys.size();
          break;
        case EmeInitDataType::KEYIDS: {
          // TODO(jrummell): Support init_data_type == "keyids".
          promise->reject(CdmPromise::Exception::NOT_SUPPORTED_ERROR, 0,
                          "init_data_type not supported.");
          return;
        }
        default:
          NOTREACHED();
          promise->reject(CdmPromise::Exception::NOT_SUPPORTED_ERROR, 0,
                          "init_data_type not supported.");
          return;
      }

      if (keys.size() > 0) {
        message = keys[0];
      }
    }

    uint32_t promise_id = cdm_promise_adapter_.SavePromise(std::move(promise));
    int requestId = nextKeyRequestId++;
    promises_[requestId] = promise_id;
    keyRequests_[requestId] = session_id;

    request_data.id = session_id;
    request_data.key_rqst_id = requestId;
    request_data.scheme = scheme_;
    if (init_data.size()) {
      request_data.init_data = message;
    }
  } else {
    // Not ClearKey - that is Widevine
    uint32_t promise_id = cdm_promise_adapter_.SavePromise(std::move(promise));
    int requestId = nextKeyRequestId++;
    promises_[requestId] = promise_id;
    keyRequests_[requestId] = session_id;

    request_data.id = session_id;
    request_data.key_rqst_id = requestId;
    request_data.scheme = scheme_;
    if (init_data.size()) {
      std::vector<uint8_t> pssh_data;
      if (!GetPsshData(init_data, &kWidevineUuid[0], &pssh_data)) {
        promise->reject(CdmPromise::Exception::NOT_SUPPORTED_ERROR, 0,
                        "Widevine init data not found.");
        return;
      }
      request_data.init_data.assign(pssh_data.begin(), pssh_data.end());
    }
  }

  LOG(INFO) << "CreateSession: " << session_id_str << " "
            << request_data.key_rqst_id;
  proxyInterface->GenerateKeyRequest(request_data);
}

// This is invoked with the response from the nzos client
void NzosDecryptor::KeyRequest(uint32_t id,
                               uint32_t keyRqstId,
                               std::vector<uint8_t> opaque_data,
                               std::string url) {
  auto promise_iter = promises_.find(keyRqstId);
  auto keyRequest_iter = keyRequests_.find(keyRqstId);
  if (promise_iter == promises_.end()) {
    LOG(ERROR) << "Cannot find promise for : " << keyRqstId;
    return;
  }

  int sessionId = keyRequest_iter->second;
  keyRequests_.erase(keyRequest_iter);

  LOG(INFO) << "Key Request received for: " << keyRqstId;
  LOG(INFO) << "Key Request received for: " << id;
  LOG(INFO) << "Key Request received for: " << sessionId;

  std::string sessionId_str = base::NumberToString(sessionId);

  updateMap_[sessionId_str] = keyRqstId;

  char web_session_id[20];
  sprintf(web_session_id, "%s", sessionId_str.c_str());

  LOG(INFO) << "Key request sent to app for " << web_session_id;

  cdm_promise_adapter_.ResolvePromise(promise_iter->second, sessionId_str);

  promises_.erase(promise_iter);

  session_message_cb_.Run(web_session_id, CdmMessageType::LICENSE_RELEASE,
                          opaque_data);
}

void NzosDecryptor::LoadSession(CdmSessionType session_type,
                                const std::string& web_session_id,
                                std::unique_ptr<NewSessionCdmPromise> promise) {
  // TODO(xhwang): Change this to NOTREACHED() when blink checks for key systems
  // that do not support loadSession. See http://crbug.com/342481
  promise->reject(CdmPromise::Exception::NOT_SUPPORTED_ERROR, 0,
                  "LoadSession() is not supported.");
}

void NzosDecryptor::UpdateSession(const std::string& session_id,
                                  const std::vector<uint8_t>& response,
                                  std::unique_ptr<SimpleCdmPromise> promise) {
  CHECK(!response.empty());

  LOG(ERROR) << "UpdateSession " << session_id;

  // TODO(jrummell): Convert back to a DCHECK once prefixed EME is removed.
  if (open_sessions_.find(session_id) == open_sessions_.end()) {
    LOG(ERROR) << "Session does not exist.";
    promise->reject(CdmPromise::Exception::INVALID_STATE_ERROR, 0,
                    "Session does not exist.");
    return;
  }

  auto iter = updateMap_.find(session_id);
  if (iter == updateMap_.end()) {
    LOG(ERROR) << "Could not find keyrequest id for session " << session_id;
    promise->reject(CdmPromise::Exception::INVALID_STATE_ERROR, 0,
                    "Session does not exist.");

  }
  uint32_t keyRequestId = (*iter).second;
  updateMap_.erase(iter);

  bool key_added = false;
  if (scheme_ == e_QzPropertyDrmScheme_ClearKey) {
    LOG(ERROR) <<"SJSJ";
    std::string key_string(response.begin(), response.end());

    KeyIdAndKeyPairs keys;
    CdmSessionType session_type = CdmSessionType::kTemporary;
    if (!ExtractKeysFromJWKSet(key_string, &keys, &session_type)) {
      LOG(ERROR) << "Response is not a valid JSON Web Key Set.";
      promise->reject(CdmPromise::Exception::INVALID_STATE_ERROR, 0,
                      "Response is not a valid JSON Web Key Set.");
      return;
    }
    LOG(ERROR) <<"SJSJ";

    // Make sure that at least one key was extracted.
    if (keys.empty()) {
      promise->reject(CdmPromise::Exception::INVALID_STATE_ERROR, 0,
                      "Response does not contain any keys.");
      return;
    }
    LOG(ERROR) <<"SJSJ";

    for (KeyIdAndKeyPairs::iterator it = keys.begin(); it != keys.end(); ++it) {
      if (it->second.length() !=
          static_cast<size_t>(DecryptConfig::kDecryptionKeySize)) {
        DVLOG(1) << "Invalid key length: " << it->second.length();
        promise->reject(CdmPromise::Exception::INVALID_STATE_ERROR, 0,
                        "Invalid key length.");
        return;
      }

      // If this key_id doesn't currently exist in this session,
      // a new key is added.
      if (!HasKey(session_id, it->first))
        key_added = true;

      if (!AddDecryptionKey(session_id, it->first, it->second)) {
        promise->reject(CdmPromise::Exception::INVALID_STATE_ERROR, 0,
                        "Unable to add key.");
        return;
      } else {
        // int sess_id;
        // base::HexStringToInt(session_id, &sess_id);
        LOG(ERROR) <<"SJSJ";
        int sess_id;
        base::StringToInt(session_id, &sess_id);
        LOG(ERROR) << "Update Key " << sess_id;
        Nz_Key_Data key_data;
        key_data.id = sess_id;

        key_data.key_rqst_id = keyRequestId;

        key_data.init_data.insert(key_data.init_data.begin(), it->first.c_str(),
                                  it->first.c_str() + it->first.length());
        key_data.key_data.insert(key_data.key_data.begin(), it->second.c_str(),
                                 it->second.c_str() + it->second.length());

        proxyInterface->UpdateSession(key_data);
      }
    }
  } else {
    int sess_id;
    base::StringToInt(session_id, &sess_id);
    LOG(ERROR) << "Update Key " << sess_id;
    Nz_Key_Data key_data;
    key_data.id = sess_id;

    key_data.key_rqst_id = keyRequestId;
    key_data.key_data.assign(response.begin(), response.end());

    proxyInterface->UpdateSession(key_data);
  }

  {
    base::AutoLock auto_lock(new_key_cb_lock_);

    if (!new_audio_key_cb_.is_null())
      new_audio_key_cb_.Run();

    if (!new_video_key_cb_.is_null())
      new_video_key_cb_.Run();
  }

  promise->resolve();

  // Create the list of all available keys for this session.

  if (scheme_ == e_QzPropertyDrmScheme_ClearKey) {
    CdmKeysInfo keys_info =
        GenerateKeysInfoList(session_id, CdmKeyInformation::RELEASED);

    session_keys_change_cb_.Run(session_id, key_added, std::move(keys_info));
  }
}

void NzosDecryptor::CloseSession(const std::string& web_session_id,
                                 std::unique_ptr<SimpleCdmPromise> promise) {
  // Validate that this is a reference to an active session and then forget it.
  auto it = open_sessions_.find(web_session_id);
  open_sessions_.erase(it);


  int sess_id;
  base::StringToInt(web_session_id, &sess_id);

  auto it2 = nz_decryptors_.find(sess_id);
  nz_decryptors_.erase(it2);

  LOG(INFO) << "CloseSession: " << sess_id;

  // Close the session.
  DeleteKeysForSession(web_session_id);
  promise->resolve();
  session_closed_cb_.Run(web_session_id);
}

void NzosDecryptor::RemoveSession(const std::string& web_session_id,
                                  std::unique_ptr<SimpleCdmPromise> promise) {
  // AesDecryptor doesn't keep any persistent data, so this should be
  // NOT_REACHED().
  // TODO(jrummell): Make sure persistent session types are rejected.
  // http://crbug.com/384152.
  //
  // However, v0.1b calls to CancelKeyRequest() will call this, so close the
  // session, if it exists.
  // TODO(jrummell): Remove the close() call when prefixed EME is removed.
  // http://crbug.com/249976.
  if (open_sessions_.find(web_session_id) != open_sessions_.end()) {
    CloseSession(web_session_id, std::move(promise));
    return;
  }

  promise->reject(CdmPromise::Exception::QUOTA_EXCEEDED_ERROR, 0,
                  "Session does not exist.");
}

CdmContext* NzosDecryptor::GetCdmContext() {
  return this;
}

std::unique_ptr<CallbackRegistration> NzosDecryptor::RegisterNewKeyCB(
    base::RepeatingClosure new_key_cb) {
  NOTIMPLEMENTED();
  return nullptr;
}

Decryptor* NzosDecryptor::GetDecryptor() {
  return this;
}

int NzosDecryptor::GetCdmId() const {
  return kInvalidCdmId;
}

void NzosDecryptor::RegisterNewKeyCB(StreamType stream_type,
                                     const NewKeyCB& new_key_cb) {
  base::AutoLock auto_lock(new_key_cb_lock_);

  switch (stream_type) {
    case kAudio:
      LOG(INFO) << "RegisterNewKeyCB - Audio";
      new_audio_key_cb_ = new_key_cb;
      break;
    case kVideo:
      LOG(INFO) << "RegisterNewKeyCB - Video";
      new_video_key_cb_ = new_key_cb;
      break;
    default:
      NOTREACHED();
  }
}

void NzosDecryptor::Decrypt(StreamType stream_type,
                            const scoped_refptr<DecoderBuffer> encrypted,
                            const DecryptCB& decrypt_cb) {
  CHECK(encrypted->decrypt_config());

  scoped_refptr<DecoderBuffer> decrypted;

  if (!encrypted->decrypt_config()) {
    // If there is no DecryptConfig, then the data is unencrypted so return it
    // immediately.
    decrypt_cb.Run(kSuccess, encrypted);
    return;
  }

  VLOG(1) << "Decrypt: " << (stream_type == kAudio ? "AUDIO" : "VIDEO");

  decrypted = DecryptDataNz(*encrypted.get(), GetInstanceId(), scheme_);
  if (!decrypted.get()) {
    DVLOG(1) << "Decryption failed.";
    LOG(ERROR) << "Decryption failed.";
    decrypt_cb.Run(kError, NULL);
    return;
  }

  decrypted->set_timestamp(encrypted->timestamp());
  decrypted->set_duration(encrypted->duration());
  decrypt_cb.Run(kSuccess, std::move(decrypted));
}

void NzosDecryptor::CancelDecrypt(StreamType stream_type) {
  // Decrypt() calls the DecryptCB synchronously so there's nothing to cancel.
  // TODO: This is likely not true for nzos.
}

void NzosDecryptor::InitializeAudioDecoder(const AudioDecoderConfig& config,
                                           const DecoderInitCB& init_cb) {
  // NzosDecryptor does not support audio decoding.
  init_cb.Run(false);
}

void NzosDecryptor::InitializeVideoDecoder(const VideoDecoderConfig& config,
                                           const DecoderInitCB& init_cb) {
  // NzosDecryptor does not support video decoding.
  init_cb.Run(false);
}

void NzosDecryptor::DecryptAndDecodeAudio(
    const scoped_refptr<DecoderBuffer> encrypted,
    const AudioDecodeCB& audio_decode_cb) {
  NOTREACHED() << "NzosDecryptor does not support audio decoding";
}

void NzosDecryptor::DecryptAndDecodeVideo(
    const scoped_refptr<DecoderBuffer> encrypted,
    const VideoDecodeCB& video_decode_cb) {
  NOTREACHED() << "NzosDecryptor does not support video decoding";
}

void NzosDecryptor::ResetDecoder(StreamType stream_type) {
  NOTREACHED() << "NzosDecryptor does not support audio/video decoding";
}

void NzosDecryptor::DeinitializeDecoder(StreamType stream_type) {
  NOTREACHED() << "NzosDecryptor does not support audio/video decoding";
}

void NzosDecryptor::SetInstanceId(uint32_t id) {
  ContentDecryptionModule::SetInstanceId(id);

  // LOG(ERROR) << "SJSJ " << id;

  // nz_decryptors_[id] = this;

  // SendCreate(id);
}

void NzosDecryptor::SendCreate(int id) {
  Nz_Decrypt_Create create_data;
  create_data.id = id;
  create_data.scheme = scheme_;

  proxyInterface->CreateDecryptor(create_data);
}

bool NzosDecryptor::CreateSession(const std::string& session_id,
                                  CdmSessionType session_type) {
  auto it = open_sessions_.find(session_id);
  if (it != open_sessions_.end())
    return false;

  auto result = open_sessions_.emplace(session_id, session_type);
  return result.second;
}

bool NzosDecryptor::AddDecryptionKey(const std::string& session_id,
                                     const std::string& key_id,
                                     const std::string& key_string) {
  std::unique_ptr<DecryptionKey> decryption_key(new DecryptionKey(key_string));
  if (!decryption_key->Init()) {
    DVLOG(1) << "Could not initialize decryption key.";
    return false;
  }

  base::AutoLock auto_lock(key_map_lock_);
  auto key_id_entry = key_map_.find(key_id);
  if (key_id_entry != key_map_.end()) {
    key_id_entry->second->Insert(session_id, std::move(decryption_key));
    return true;
  }

  // |key_id| not found, so need to create new entry.
  std::unique_ptr<SessionIdDecryptionKeyMap> inner_map(
      new SessionIdDecryptionKeyMap());
  inner_map->Insert(session_id, std::move(decryption_key));
  key_map_[key_id] = std::move(inner_map);
  return true;
}

NzosDecryptor::DecryptionKey* NzosDecryptor::GetKey_Locked(
    const std::string& key_id) const {
  key_map_lock_.AssertAcquired();
  auto key_id_found = key_map_.find(key_id);
  if (key_id_found == key_map_.end())
    return NULL;

  // Return the key from the "latest" session_id entry.
  return key_id_found->second->LatestDecryptionKey();
}

bool NzosDecryptor::HasKey(const std::string& session_id,
                           const std::string& key_id) {
  base::AutoLock auto_lock(key_map_lock_);
  KeyIdToSessionKeysMap::const_iterator key_id_found = key_map_.find(key_id);
  if (key_id_found == key_map_.end())
    return false;

  return key_id_found->second->Contains(session_id);
}

void NzosDecryptor::DeleteKeysForSession(const std::string& session_id) {
  base::AutoLock auto_lock(key_map_lock_);

  // Remove all keys associated with |session_id|. Since the data is
  // optimized for access in GetKey_Locked(), we need to look at each entry in
  // |key_map_|.
  auto it = key_map_.begin();
  while (it != key_map_.end()) {
    it->second->Erase(session_id);
    if (it->second->Empty()) {
      // Need to get rid of the entry for this key_id. This will mess up the
      // iterator, so we need to increment it first.
      auto current = it;
      ++it;
      key_map_.erase(current);
    } else {
      ++it;
    }
  }
}

NzosDecryptor::DecryptionKey::DecryptionKey(const std::string& secret)
    : secret_(secret) {}

NzosDecryptor::DecryptionKey::~DecryptionKey() {}

bool NzosDecryptor::DecryptionKey::Init() {
  CHECK(!secret_.empty());
  decryption_key_ =
      crypto::SymmetricKey::Import(crypto::SymmetricKey::AES, secret_);
  if (!decryption_key_)
    return false;
  return true;
}

CdmKeysInfo NzosDecryptor::GenerateKeysInfoList(
    const std::string& session_id,
    CdmKeyInformation::KeyStatus status) {
  // Create the list of all available keys for this session.
  CdmKeysInfo keys_info;
  {
    base::AutoLock auto_lock(key_map_lock_);
    for (const auto& item : key_map_) {
      if (item.second->Contains(session_id)) {
        keys_info.push_back(
            std::make_unique<CdmKeyInformation>(item.first, status, 0));
      }
    }
  }
  return keys_info;
}

}  // namespace media
