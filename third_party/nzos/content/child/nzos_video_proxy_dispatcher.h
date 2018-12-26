// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_CHILD_NZ_VIDEO_PROXY_NZ_VIDEO_PROXY_DISPATCHER_H_
#define CONTENT_CHILD_NZ_VIDEO_PROXY_NZ_VIDEO_PROXY_DISPATCHER_H_

#include <list>
#include <map>
#include "base/memory/scoped_refptr.h"
#include "base/time/time.h"
#include "base/timer/timer.h"
#include "content/common/content_export.h"
#include "ipc/message_filter.h"
#include "third_party/nzos/media/nzos_media_proxy_interface.h"

namespace base {
class MessageLoop;
}

namespace content {

class NzMsgDelay {
public:
    NzMsgDelay(int request_id, base::Time last_time, IPC::Message* msg, uint64_t bits);
    ~NzMsgDelay() {}
        int request_id_;
    base::Time insert_time_;
    base::Time last_time_;
    IPC::Message* msg_;
    uint64_t bits_;
};

class NzBwData {
public:
    NzBwData(double current_bw);
    ~NzBwData() {}
    double current_bw_;
    base::Time last_time_;
        base::Time last_sent_time_;
    int data_points_;
};

class ResourceDispatcher;

class CONTENT_EXPORT NzVideoProxyDispatcher : public IPC::MessageFilter, media::NzosMediaProxyInterface {
 public:
  explicit NzVideoProxyDispatcher(
	  const scoped_refptr<base::SingleThreadTaskRunner>& ipc_task_runner,
      // ResourceDispatcher* resource_dispatcher,
      IPC::Sender* sender);

  static NzVideoProxyDispatcher* Instance() { return s_Instance; };

  int RenderId() override;

  void Create(const Nz_Proxy_Create& create_data) override;
  void Start(const Nz_Proxy_Initial_Data& init_data) override;
  void Update(const Nz_Proxy_Initial_Data& init_data) override; 
  void BoundingRect(const Nz_Proxy_Bounding_Rect& bounding_rect) override;
  void Play(int id, double duration) override;
  void Pause(const Nz_Proxy_Id& id_data) override;
  void Reset(const Nz_Proxy_Id& id_data) override;
  void Stop(const Nz_Proxy_Id& id_data) override;
  void Destroy(const Nz_Proxy_Id& id_data) override;
  void Remove(const Nz_Proxy_Id& id_data) override;
  void Restore(const Nz_Proxy_Id& id_data) override;
  void Buffer(const Nz_Proxy_Media_Buffer& buffer) override;
  void Hidden(const Nz_Proxy_Id& id_data) override;
  void Shown(const Nz_Proxy_Id& id_data) override;
  void Seek(const Nz_Proxy_Seek& seek_data) override;

  virtual void ScrollVector(const int X, const int Y);

  bool CapabilitiesRcvd() { return capabilities_rcvd_; }
  bool H264Capable() override;
  bool Vp8Capable() override;
  bool Vp9Capable() override;
  bool ClearkeyCapable() { return clearkey_capable_; }
  bool NzEncryptionCapable() { return nz_encrypt_capable_; }
  bool WidevineCapable() { return widevine_capable_; }

  bool OnMessageReceived(const IPC::Message& message) override;

  bool Detect360VideoByCanvas() { return detect360VideoByCanvas_; }
  const std::vector<std::string>& GetDisabledIds() { return disable_by_ids_; }
  const std::vector<std::string>& GetDisabledUrls() { return disable_by_urls_; }
  const std::string& GetBypassUrl() { return bypass_url_; }
  const std::string& GetBypassAttr() { return bypass_attr_; }

 protected:
  ~NzVideoProxyDispatcher() override;

 private:
  static NzVideoProxyDispatcher* s_Instance;

   // Sends an IPC message using |channel_|.
  void Send(IPC::Message* message);

  // IPC::MessageFilter override. Called on |io_message_loop|.
  void OnFilterAdded(IPC::Channel* sender) override;
  void OnFilterRemoved() override;
  void OnChannelClosing() override; // IPC channel for Send(); must only be accessed on |io_message_loop_|.

  virtual void OnRenderId(const Nz_Proxy_Id& id_data);
  virtual void OnCapabilities(const Nz_Capabilities& capability_data);
  virtual void OnClientBandwidth(const Nz_Client_Bandwidth& bandwidth_data);
  virtual void OnKeyRequest(const Nz_Key_Request& key_request);
  virtual bool HandleDataMsg(const IPC::Message& message);
  virtual bool HandleCompleteMsg(const IPC::Message& message);
  virtual void PostMsg();

  void DestroyOnIOThread(const Nz_Proxy_Id& id_data);
  void RemoveOnIOThread(const Nz_Proxy_Id& id_data);
  void RestoreOnIOThread(const Nz_Proxy_Id& id_data);
  void HiddenOnIOThread(const Nz_Proxy_Id& id_data);
  void ShownOnIOThread(const Nz_Proxy_Id& id_data);
  void SeekOnIOThread(const Nz_Proxy_Seek& seek_data);
  void ScrollVectorOnIOThread(const int X, const int Y);

 public:
  void AudioCreate(const Nz_Proxy_Create& create_data) override;
  void AudioStart(const Nz_Audio_Initial_Data& init_data) override;
  void AudioBuffer(const Nz_Proxy_Media_Buffer& buffer) override;
  void AudioVolume(const Nz_Audio_Volume& volume_data) override;
  void AudioDestroy(const Nz_Proxy_Id& id_data) override;

public:
  virtual void CreateDecryptor(const Nz_Decrypt_Create& create_data);
  virtual void GenerateKeyRequest(const Nz_Generate_Key_Request& request_data);
  virtual void UpdateSession(const Nz_Key_Data& key_data);
  virtual void ReleaseSession(const Nz_Session_Release& session_data);

private:
  void CreateDecryptorOnIOThread(const Nz_Decrypt_Create& create_data);
  void GenerateKeyRequestOnIOThread(const Nz_Generate_Key_Request& request_data);
  void UpdateSessionOnIOThread(const Nz_Key_Data& key_data);
  void ReleaseSessionOnIOThread(const Nz_Session_Release& session_data);

public:
  virtual void NzLog(int severity, const std::string& log_msg);
private:
  virtual void NzLogOnIOThread(int severity, const std::string& log_msg);

  virtual void OnDeviceProperties(bool allowUserAgentChange, bool isMobileDevice, std::string deviceSw);

  double calcDelay(int64_t bits);

  IPC::Sender* sender_ = nullptr;

  // // Message loop on which IPC calls are driven.
  const scoped_refptr<base::SingleThreadTaskRunner>& ipc_task_runner_;

  // ResourceDispatcher* resource_dispatcher_;

  bool capabilities_rcvd_;
  bool h264_capable_;
  bool vp8_capable_;
  bool vp9_capable_;
  bool clearkey_capable_;
  bool nz_encrypt_capable_;
  bool widevine_capable_;
  int render_id_;
  int active_nz_decoders_;
  base::OneShotTimer timer_;
  double target_bw_;
  double current_bw_;
  bool reset_bw_calc_;
  std::list<NzMsgDelay*>rrDelay_;
  std::map<int, NzBwData*> bw_data_;
  std::vector<std::string> disable_by_ids_;
  std::vector<std::string> disable_by_urls_;
  std::string bypass_url_;
  std::string bypass_attr_;
  bool detect360VideoByCanvas_;

  DISALLOW_COPY_AND_ASSIGN(NzVideoProxyDispatcher);
};

}  // namespace content

#endif  // CONTENT_RENDERER_BATTERY_STATUS_BATTERY_STATUS_DISPATCHER_H_

