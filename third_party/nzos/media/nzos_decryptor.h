// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef MEDIA_CRYPTO_NZ_DECRYPTOR_H_
#define MEDIA_CRYPTO_NZ_DECRYPTOR_H_

#include <stdint.h>

#include <map>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

#include "base/macros.h"
#include "base/memory/ref_counted.h"
#include "base/synchronization/lock.h"
#include "base/thread_annotations.h"
#include "media/base/cdm_context.h"
#include "media/base/cdm_key_information.h"
#include "media/base/cdm_promise.h"
#include "media/base/cdm_promise_adapter.h"
#include "media/base/content_decryption_module.h"
#include "media/base/decryptor.h"
#include "media/base/media_export.h"
#include "third_party/nzos/media/nzos_media_proxy_interface.h"

namespace crypto {
  class SymmetricKey;
}

namespace media {

// Decrypts an AES encrypted buffer into an unencrypted buffer. The AES
// encryption must be CTR with a key size of 128bits.
  class MEDIA_EXPORT NzosDecryptor : public ContentDecryptionModule,
                                     public CdmContext,
                                     public Decryptor {
  public:
    NzosDecryptor (const SessionMessageCB& session_message_cb,
                   const SessionClosedCB& session_closed_cb,
                   const SessionKeysChangeCB& session_keys_change_cb,
                   const SessionExpirationUpdateCB& session_expiration_update_cb);
    ~NzosDecryptor () override;

    // NzOS specific
    static void SetProxyInterface(NzosMediaProxyInterface* inst);

    static bool NzosAesCapable ();
    static NzosDecryptor* getNzosDecryptor (int id);
    static uint32_t SessionIdFromKeyId (const std::string& key_id);
    void KeyRequest (uint32_t sessionId, uint32_t keyRqstId,
                     std::vector<uint8_t> opaque_data, std::string url);

    // ContentDecryptionModule implementation.
    void SetServerCertificate (
      const std::vector<uint8_t>& certificate,
      std::unique_ptr<SimpleCdmPromise> promise) override;
    void CreateSessionAndGenerateRequest (
        CdmSessionType session_type,
        EmeInitDataType init_data_type,
        const std::vector<uint8_t>& init_data,
        std::unique_ptr<NewSessionCdmPromise> promise) override;
    void LoadSession (CdmSessionType session_type, 
                      const std::string& web_session_id,
                      std::unique_ptr<NewSessionCdmPromise> promise) override;
    void UpdateSession (const std::string& session_id,
                        const std::vector<uint8_t>& response,
                        std::unique_ptr<SimpleCdmPromise> promise) override;
    void CloseSession (const std::string& web_session_id,
                       std::unique_ptr<SimpleCdmPromise> promise) override;
    void RemoveSession (const std::string& web_session_id,
                        std::unique_ptr<SimpleCdmPromise> promise) override;
    CdmContext* GetCdmContext () override;

    // CdmContext implementation
    std::unique_ptr<CallbackRegistration> RegisterNewKeyCB(
        base::RepeatingClosure new_key_cb) override;
    Decryptor* GetDecryptor () override;
    int GetCdmId () const override;

    // Decryptor implementation.
    void RegisterNewKeyCB (StreamType stream_type,
                           const NewKeyCB& key_added_cb) override;
    void Decrypt (StreamType stream_type, 
                  scoped_refptr<DecoderBuffer> encrypted,
                  const DecryptCB& decrypt_cb) override;
    void CancelDecrypt (StreamType stream_type) override;
    void InitializeAudioDecoder (const AudioDecoderConfig& config,
                                 const DecoderInitCB& init_cb) override;
    void InitializeVideoDecoder (const VideoDecoderConfig& config,
                                 const DecoderInitCB& init_cb) override;
    void DecryptAndDecodeAudio(scoped_refptr<DecoderBuffer> encrypted,
                              const AudioDecodeCB& audio_decode_cb) override;
    void DecryptAndDecodeVideo(scoped_refptr<DecoderBuffer> encrypted,
                              const VideoDecodeCB& video_decode_cb) override;
    void ResetDecoder (StreamType stream_type) override;
    void DeinitializeDecoder (StreamType stream_type) override;

    int GetDrmScheme () override;

    void SetInstanceId(uint32_t id) override;

  private:
    // TODO(fgalligan): Remove this and change KeyMap to use crypto::SymmetricKey
    // as there are no decryptors that are performing an integrity check.
    // Helper class that manages the decryption key.
    class DecryptionKey {
    public:
      explicit DecryptionKey (const std::string& secret);
      ~DecryptionKey ();

      // Creates the encryption key.
      bool Init ();

      crypto::SymmetricKey* decryption_key () { return decryption_key_.get (); }

    private:
      // The base secret that is used to create the decryption key.
      const std::string secret_;

      // The key used to decrypt the data.
      std::unique_ptr<crypto::SymmetricKey> decryption_key_;

      DISALLOW_COPY_AND_ASSIGN(DecryptionKey);
    };

    // Keep track of the keys for a key ID. If multiple sessions specify keys
    // for the same key ID, then the last key inserted is used. The structure is
    // optimized so that Decrypt() has fast access, at the cost of slow deletion
    // of keys when a session is released.
    class SessionIdDecryptionKeyMap;

    // Key ID <-> SessionIdDecryptionKeyMap map.
    using KeyIdToSessionKeysMap =
      std::unordered_map<std::string,
                         std::unique_ptr<SessionIdDecryptionKeyMap>>;

    // Creates a DecryptionKey using |key_string| and associates it with |key_id|.
    // Returns true if successful.
    bool AddDecryptionKey (const std::string& web_session_id,
                           const std::string& key_id,
                           const std::string& key_string);

    // Gets a DecryptionKey associated with |key_id|. The AesDecryptor still owns
    // the key. Returns NULL if no key is associated with |key_id|.
    DecryptionKey* GetKey_Locked (const std::string& key_id) const
        EXCLUSIVE_LOCKS_REQUIRED(key_map_lock_);

    // Determines if |key_id| is already specified for |session_id|.
    bool HasKey (const std::string& session_id, const std::string& key_id);

    // Deletes all keys associated with |web_session_id|.
    void DeleteKeysForSession (const std::string& web_session_id);

    CdmKeysInfo GenerateKeysInfoList(const std::string& session_id,
                                     CdmKeyInformation::KeyStatus status);

    // Callbacks for firing session events.
    SessionMessageCB session_message_cb_;
    SessionClosedCB session_closed_cb_;
    SessionKeysChangeCB session_keys_change_cb_;
    SessionExpirationUpdateCB session_expiration_update_cb_;

    // Since only Decrypt() is called off the renderer thread, we only need to
    // protect |key_map_|, the only member variable that is shared between
    // Decrypt() and other methods.
    mutable base::Lock key_map_lock_;
    KeyIdToSessionKeysMap key_map_;  // Protected by |key_map_lock_|.

    // Keeps track of current open sessions and their type. Although publicly
    // AesDecryptor only supports temporary sessions, ClearKeyPersistentSessionCdm
    // uses this class to also support persistent sessions, so save the
    // CdmSessionType for each session.
    std::map<std::string, CdmSessionType> open_sessions_;
    CdmPromiseAdapter cdm_promise_adapter_;

    // Make web session ID unique per renderer by making it static. Web session
    // IDs seen by the app will be "1", "2", etc.
    static uint32_t next_session_id_;
    static int ids;

    mutable base::Lock new_key_cb_lock_;
    NewKeyCB new_audio_key_cb_ GUARDED_BY(new_key_cb_lock_);
    NewKeyCB new_video_key_cb_ GUARDED_BY(new_key_cb_lock_);

    using PromiseMap = std::unordered_map<uint32_t, uint32_t>;

    // Storage for promises from invocation to response from client.
    // CdmPromiseAdapter cdm_promise_adapter_;
    PromiseMap promises_;

    static std::map<int, NzosDecryptor*> nz_decryptors_;
    static NzosMediaProxyInterface* proxyInterface;

    DISALLOW_COPY_AND_ASSIGN(NzosDecryptor);
  };

}  // namespace media

#endif  // MEDIA_CRYPTO_NZ_DECRYPTOR_H_
