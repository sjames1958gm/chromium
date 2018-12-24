  // Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_BROWSER_NZ_VIDEO_PROXY_MESSAGE_FILTER_H_
#define CONTENT_BROWSER_NZ_VIDEO_PROXY_MESSAGE_FILTER_H_

#include <fstream>
#include <map>
#include <list>
#include "base/time/time.h"
#include "base/timer/timer.h"
#include "content/public/browser/browser_message_filter.h"
#include "third_party/nzos/include/nzos_media_proxy_messages.h"

namespace content {

class ResourceMessageFilter;

class NzStream {
public:
  NzStream (int routing_id, uint32_t key_sess, int other_stream_id,
            uint32_t drm_scheme,
            const std::string& bypassUrl, const std::string& bypassCorr);
  virtual
  ~NzStream ();

  int routing_id;
  int other_stream_id_;
  int codec_;
  void* handle;
  uint32_t* pfb;
  uint32_t video_width;
  uint32_t video_height;
  bool rectSet;
  gfx::Rect rect;
  int offset;
  bool is_started;
  bool is_shown;
  bool is_capturing;
  bool need_send_key;
  bool encrypt_video_;
  std::ofstream fs;
  void *oaes_ctx;
  std::size_t key_len;
  uint8_t *key;
  uint32_t key_session;
  int64_t duration_;
  uint32_t drm_scheme_;
  bool is_playing_;
  double mediaDuration_;
  std::string bypassUrl_;
  std::string bypassCorr_;
  Nz_Proxy_Initial_Data init_data;
};

class NzAudioStream {
public:
  NzAudioStream (int routing_id, int other_stream_id, uint32_t drm_scheme, const std::string& bypassUrl,
      const std::string& bypassCorr);
  virtual
  ~NzAudioStream ();

  int routing_id_;
  int other_stream_id_;
  uint32_t volume_;
  uint32_t drm_scheme_;
  void* handle;
  uint32_t codec_;
  uint32_t rate_;
  uint32_t format_;
  uint32_t channels_;
  bool paused_;
  double mediaDuration_;
  std::string bypassUrl_;
  std::string bypassCorr_;
  std::vector<unsigned char> extradata_;
};

class NzDRMSession {
public:
  NzDRMSession (int id, int session, int scheme);
  ~NzDRMSession ();
  uint32_t id_;
  uint32_t session_;
  uint32_t scheme_;
};

class NzVideoProxyMessageFilterPlatformInterface;

class NzVideoProxyMessageFilter : public BrowserMessageFilter {
public:
  NzVideoProxyMessageFilter (int render_process_id); //, ResourceMessageFilter* rmf);

  static NzVideoProxyMessageFilter* GetInstanceFromSession (uint32_t u32SessionId);
  static bool OnKeyMessageReceivedS (uint32_t u32SessionId, uint32_t u32KeyRqstId,
                         const uint8_t* pOpaqueData, uint32_t u32OpaqueDataLen,
                         const char* url, uint32_t u32UrlLen);

  static void OnDevicePropertiesReceivedS (void* pNzDevice);
  static void StaticInitialization();
  static NzVideoProxyMessageFilterPlatformInterface* platformInterface_;

  // BrowserMessageFilter implementation.
  bool OnMessageReceived (const IPC::Message& message) override;
  void OnFilterAdded (IPC::Channel* sender) override; 

  // Events from nzos platform thread
  void OnKeyMessageReceived (uint32_t u32SessionId, uint32_t u32KeyRqstId,
                        const uint8_t* pOpaqueData, uint32_t u32OpaqueDataLen,
                        const char* url, uint32_t u32UrlLen);
  virtual void OnDevicePropertiesReceived (void* pNzDevice);

private:

  ~NzVideoProxyMessageFilter () override;

  void SetEncryptionState (void* pNzDevice = NULL);

  void OnKeyMessageReceivedUIThread (uint32_t u32SessionId, uint32_t u32KeyRqstId,
                                const uint8_t* pOpaqueData,
                                uint32_t u32OpaqueDataLen, std::string url);

  void HideOthers(const NzStream* s);
  void ShowOther(const NzStream* s);

  void OnCreate (const Nz_Proxy_Create& create);
  void OnCreateUIThread (const Nz_Proxy_Create& create);

  void OnStart (const Nz_Proxy_Initial_Data& init_data);
  void OnStartUIThread (const Nz_Proxy_Initial_Data& init_data);

  void OnUpdate (const Nz_Proxy_Initial_Data& init_data);
  void OnUpdateUIThread (const Nz_Proxy_Initial_Data& init_data);

  void OnBoundingRect (const Nz_Proxy_Bounding_Rect& bounding_rect);
  void OnBoundingRectUIThread (const Nz_Proxy_Bounding_Rect& bounding_rect);

  void OnPlay (int id, double duration);
  void OnPlayUIThread (int id, double duration);

  void OnPause (const Nz_Proxy_Id& id);
  void OnPauseUIThread (const Nz_Proxy_Id& id);

  void OnReset (const Nz_Proxy_Id& id);
  void OnResetUIThread (const Nz_Proxy_Id& id);

  void OnStop (const Nz_Proxy_Id& id);
  void OnStopUIThread (const Nz_Proxy_Id& id);

  void OnDestroy (const Nz_Proxy_Id& id);
  void OnDestroyUIThread (const Nz_Proxy_Id& id);

  void OnBuffer (const Nz_Proxy_Media_Buffer& buffer);
  void OnBufferUIThread (const Nz_Proxy_Media_Buffer& buffer);

  void OnHidden (const Nz_Proxy_Id& id);
  void OnHiddenUIThread (const Nz_Proxy_Id& id);

  void OnShown (const Nz_Proxy_Id& id);
  void OnShownUIThread (const Nz_Proxy_Id& id);

  void OnSeek(const Nz_Proxy_Seek& seek_data);
  void OnSeekUIThread(const Nz_Proxy_Seek& seek_data);

  void OnRemove(const Nz_Proxy_Id& id);
  void OnRemoveUIThread(const Nz_Proxy_Id& id);

  void OnRestore(const Nz_Proxy_Id& id);
  void OnRestoreUIThread(const Nz_Proxy_Id& id);

  void OnAudioCreate (const Nz_Proxy_Create& create);
  void OnAudioCreateUIThread (const Nz_Proxy_Create& create);

  void OnAudioStart (const Nz_Audio_Initial_Data& init_data);
  void OnAudioStartUIThread (const Nz_Audio_Initial_Data& init_data);

  void OnAudioBuffer (const Nz_Proxy_Media_Buffer& buffer);
  void OnAudioBufferUIThread (const Nz_Proxy_Media_Buffer& buffer);

  void OnAudioVolume (const Nz_Audio_Volume& volume_data);
  void OnAudioVolumeUIThread (const Nz_Audio_Volume& volume_data);

  void OnAudioDestroy (const Nz_Proxy_Id& id_data);
  void OnAudioDestroyUIThread (const Nz_Proxy_Id& id_data);

  void OnEncryptionDetected (const Nz_Proxy_Id& id_data);
  void OnEncryptionDetectedUIThread (const Nz_Proxy_Id& id_data);

  void OnDecryptorCreate (const Nz_Decrypt_Create& create_data);
  void OnDecryptorCreateUIThread (const Nz_Decrypt_Create& create_data);

  void OnGenerateKeyRequest (const Nz_Generate_Key_Request& request_data);
  void OnGenerateKeyRequestUIThread (const Nz_Generate_Key_Request& request_data);

  void OnUpdateSession (const Nz_Key_Data& key_data);
  void OnUpdateSessionUIThread (const Nz_Key_Data& key_data);

  void OnReleaseSession (const Nz_Session_Release& session_data);
  void OnReleaseSessionUIThread (const Nz_Session_Release& session_data);

  void OnNzLog (int severity, const std::string& log_msg);

  void OnScrollVector (int X, int Y);

  int id_;
  int render_process_id_;
  std::map<int, NzStream*> streams_;
  std::map<int, NzDRMSession*> sessions_;
  std::map<int, NzAudioStream*> audioStreams_;
  std::string capture_filename_;
  // ResourceMessageFilter* resourceMessageFilter_;
  bool clearkey_encrypt_;
  bool widevine_encryption_;
  bool fullscreen_video_;
  std::ofstream fsaudio;

  static std::map<int, NzVideoProxyMessageFilter*>& nz_messageFilters_;
  static int ids_;

  DISALLOW_COPY_AND_ASSIGN(NzVideoProxyMessageFilter);
};

}  // namespace content

#endif  // CONTENT_BROWSER_BATTERY_STATUS_BATTERY_STATUS_MESSAGE_FILTER_H_

