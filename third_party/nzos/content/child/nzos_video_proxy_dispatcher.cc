// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "third_party/nzos/content/child/nzos_video_proxy_dispatcher.h"

#include "base/base_switches.h"
#include "base/bind.h"
#include "base/command_line.h"
#include "content/child/child_process.h"
#include "content/child/child_thread_impl.h"

// #include "content/renderer/loader/resource_dispatcher.h"
// #include "content/common/resource_messages.h"
#include "ipc/ipc_logging.h"
#include "ipc/ipc_message_macros.h"
#include "media/base/media_switches.h"
#include "third_party/nzos/media/nzos_video_decoder.h"

// TODOSJ
// #include "nzos/video_proxy/nz_decryptor.h"
// #include "nzos/video_proxy/nz_wv_decryptor.h"

// Only static functions are available in renderer
#include "ui/ozone/platform/nzos/nzos_platform_thread.h"

// #include "content/child/blink_platform_impl.h"

namespace content {

NzVideoProxyDispatcher* NzVideoProxyDispatcher::s_Instance = 0;

//#define CALC_APPARENT
#define TARGETBW 1000000
#define BWCOEFF 0.5
#define DATAPOINTS 2

bool NzLogMessageHandlerFunction(int severity,
                                 const char* file,
                                 int line,
                                 size_t message_start,
                                 const std::string& str) {
  if (NzVideoProxyDispatcher::Instance()) {
    if (str.length() > 1000) {
      std::string repstr = str.substr(0, 999);
      repstr += ". . . truncated";
      NzVideoProxyDispatcher::Instance()->NzLog(severity, repstr);
    } else {
      NzVideoProxyDispatcher::Instance()->NzLog(severity, str);
    }
  }
  return true;
}

NzMsgDelay::NzMsgDelay(int request_id,
                       base::Time last_time,
                       IPC::Message* msg,
                       uint64_t bits)
    : request_id_(request_id),
      insert_time_(base::Time::Now()),
      last_time_(last_time),
      msg_(msg),
      bits_(bits) {}

NzBwData::NzBwData(double current_bw) : current_bw_(current_bw) {
  last_time_ = base::Time::Now();
  data_points_ = 0;
}

NzVideoProxyDispatcher::NzVideoProxyDispatcher(
    const scoped_refptr<base::SingleThreadTaskRunner>& ipc_task_runner,
    // ResourceDispatcher* resource_dispatcher,
    IPC::Sender* sender)
    : //sender_(sender),
      ipc_task_runner_(ipc_task_runner),
      capabilities_rcvd_(false),
      h264_capable_(false),
      vp8_capable_(false),
      vp9_capable_(false),
      clearkey_capable_(false),
      nz_encrypt_capable_(false),
      widevine_capable_(false),
      render_id_(-1),
      active_nz_decoders_(0),
      target_bw_(TARGETBW),
      current_bw_(TARGETBW),
      reset_bw_calc_(false),
      detect360VideoByCanvas_(false)
       {

  const base::CommandLine* cmd_line = base::CommandLine::ForCurrentProcess();
  
  // TODOSJ
  if (cmd_line->HasSwitch(switches::kEnableNZBWControl)) {
    LOG(INFO) << "NZ bandwidth control enabled";
  } else {
    LOG(INFO) << "NZ bandwidth control disabled";
    // resource_dispatcher_ = 0;
  }

  // Don't capture log only if the switch is present with value "no"
  if (!cmd_line->HasSwitch(switches::kNzCaptureLog) ||
     (cmd_line->GetSwitchValueASCII(switches::kNzCaptureLog).compare("no") == 0)) {
    logging::SetLogMessageHandler(NzLogMessageHandlerFunction);
  }
  
  s_Instance = this;

  media::NZVideoDecoder::SetProxyInterface(this);
}

NzVideoProxyDispatcher::~NzVideoProxyDispatcher() {}

void NzVideoProxyDispatcher::Send(IPC::Message* message) {
  // DCHECK(ipc_task_runner_->BelongsToCurrentThread());
  // if (!sender_) {
  //   delete message;
  // } else {
  //   sender_->Send(message);
  // }
}

bool NzVideoProxyDispatcher::OnMessageReceived(const IPC::Message& message) {

  // if (message.type() >> 16 == 55) LOG(ERROR) << "SJSJ " << (message.type() & 0xffff);
  // DCHECK(ipc_task_runner_->BelongsToCurrentThread());
  bool handled = true;
  IPC_BEGIN_MESSAGE_MAP(NzVideoProxyDispatcher, message)
    IPC_MESSAGE_HANDLER(NzVideoProxyMsg_RenderID, OnRenderId)
    IPC_MESSAGE_HANDLER(NzVideoProxyMsg_Capabilities, OnCapabilities)
    IPC_MESSAGE_HANDLER(NzVideoProxyMsg_Bandwidth, OnClientBandwidth)
    IPC_MESSAGE_HANDLER(NzVideoProxyMsg_KeyRequest, OnKeyRequest)
    IPC_MESSAGE_HANDLER(NzVideoProxyMsg_DeviceProperties, OnDeviceProperties)
    // IPC_MESSAGE_HANDLER_GENERIC(ResourceMsg_DataReceived, {
    //     handled = HandleDataMsg(message)
    //     ;
    // }
    //     )
    // IPC_MESSAGE_HANDLER_GENERIC(ResourceMsg_RequestComplete, {
    //     handled = HandleCompleteMsg(message)
    //     ;
    // 
    //     )
    IPC_MESSAGE_UNHANDLED(handled = false)
  IPC_END_MESSAGE_MAP()
  return handled;
}

void NzVideoProxyDispatcher::OnFilterAdded(IPC::Channel* sender) {
  DCHECK(ipc_task_runner_->BelongsToCurrentThread());
  LOG(ERROR) << "SJSJ";
  sender_ = sender;
}

void NzVideoProxyDispatcher::OnFilterRemoved() {
  DCHECK(ipc_task_runner_->BelongsToCurrentThread());

  // Once removed, a filter will not be used again.  At this time all
  // delegates must be notified so they release their reference.
  OnChannelClosing();
}

void NzVideoProxyDispatcher::OnChannelClosing() {
  DCHECK(ipc_task_runner_->BelongsToCurrentThread());
  sender_ = NULL;
}

void NzVideoProxyDispatcher::OnRenderId(const Nz_Proxy_Id& id_data) {
  LOG(ERROR) << "OnRenderId";
  render_id_ = id_data.id;
}

void NzVideoProxyDispatcher::OnCapabilities(
    const Nz_Capabilities& capability_data) {
  capabilities_rcvd_ = true;
  h264_capable_ = capability_data.h264Capable;
  vp8_capable_ = capability_data.vp8Capable;
  vp9_capable_ = capability_data.vp9Capable;
  clearkey_capable_ = capability_data.clearkeyCapable;
  nz_encrypt_capable_ = capability_data.nzEncryptCapable;
  widevine_capable_ = capability_data.widevineCapable;

  detect360VideoByCanvas_ = capability_data.detect360bycanvas;
  disable_by_ids_ = capability_data.disableById;
  disable_by_urls_ = capability_data.disableByUrl;

  bypass_url_ = capability_data.bypassUrl;
  bypass_attr_ = capability_data.bypassAttr;

  // TODOSJ
  // content::BlinkPlatformImpl::setAppApiPort(capability_data.appApiPort);

  LOG(INFO) << "NzVideoProxyDispatcher::OnCapabilities: H.264("
            << (h264_capable_ ? "true" : "false") << "), VP8("
            << (vp8_capable_ ? "true" : "false") << "), VP9("
            << (vp9_capable_ ? "true" : "false") << ")";

  LOG(INFO) << "NzVideoProxyDispatcher::OnCapabilities: "
            << "BypassUrl: " << bypass_url_ << ", BypassAttr: " << bypass_attr_;
}

void NzVideoProxyDispatcher::OnClientBandwidth(
    const Nz_Client_Bandwidth& bandwidth_data) {
  target_bw_ = bandwidth_data.bandwidth;
}
void NzVideoProxyDispatcher::OnKeyRequest(const Nz_Key_Request& key_request) {
/* TODOSJ
        media::NzDecryptor* decryptor = media::NzDecryptor::getNzDecryptor(
                        key_request.id);
        if (decryptor) {
                return decryptor->KeyRequest(key_request.id,
key_request.key_rqst_id, key_request.opaque_data, key_request.url);
        }
        media::NzWvDecryptor* wv_decryptor =
media::NzWvDecryptor::getNzWvDecryptor( key_request.id); if (wv_decryptor) {
                return wv_decryptor->KeyRequest(key_request.id,
key_request.key_rqst_id, key_request.opaque_data, key_request.url);
        }
*/
}
bool NzVideoProxyDispatcher::HandleDataMsg(const IPC::Message& message) {
  bool handled = false;
  /*
      if (resource_dispatcher_ && (active_nz_decoders_ > 0)) {

          if (reset_bw_calc_) {
              current_bw_ = TARGETBW;
              reset_bw_calc_ = false;
          }

          base::PickleIterator iter(message);
          int request_id, data_offset, data_length;
          if (!iter.ReadInt(&request_id)
                  || !iter.ReadInt(&data_offset)
                  || !iter.ReadInt(&data_length)) {
              return handled;
          }

          std::map<int, NzBwData*>::iterator it = bw_data_.find(request_id);

          NzBwData* curr_entry = nullptr;
          base::Time last_time;

          uint64 bits = ((int64) data_length) * 8;

          if (it == bw_data_.end()) {
              curr_entry = new NzBwData(current_bw_);
              bw_data_[request_id] = curr_entry;

          } else {

              curr_entry = it->second;
              last_time = curr_entry->last_time_;

              if (!last_time.is_null() && (data_length != 0)) {
                  base::TimeDelta time_delta = base::Time::Now() -
     curr_entry->last_time_;

                  double td = time_delta.InSecondsF()    ;

                  if (td != 0) {

                      // Calculate rate in bits / second
                      double rate = bits / td;
                      // Calculate the new BW using the existing BW
                      double new_bw = (curr_entry->current_bw_ * BWCOEFF)
                              + (1.0 - BWCOEFF) * rate;

                      //LOG(ERROR) << "(" << request_id << "), data rate: "
                      //                        << rate << " bits/sec -> "
                      //                        << bits << "/" << td;
                      //LOG(ERROR) << "current BW: " << curr_entry->current_bw_
     << " new BW: " << new_bw;

                      curr_entry->current_bw_ = new_bw;
                      curr_entry->data_points_++;
                      if (curr_entry->data_points_ > DATAPOINTS) {
                          current_bw_ = curr_entry->current_bw_;
                          if ((curr_entry->data_points_ % DATAPOINTS) == 0) {
                              //LOG(ERROR) << "current BW: " << current_bw_ << "
     new BW: " << curr_entry->current_bw_;
                          }
                      }
                  }
              } else {
                  //LOG(ERROR) << "(" << request_id
                  //                        << "), data rate: --------------
     bits/sec ,"
                  //                        << data_length;
              }
              curr_entry->last_time_ = base::Time::Now();
          }


          if (rrDelay_.size() > 0) {

              NzMsgDelay* md = new NzMsgDelay(request_id, last_time, new
     IPC::Message(message), bits); rrDelay_.push_back(md); handled = true;

          } else {

              int64 delay = calcDelay(bits);

              if (delay > 0) {

                  handled = true;

                  NzMsgDelay* md = new NzMsgDelay(request_id, last_time, new
     IPC::Message(message), bits);

                  rrDelay_.push_back(md);

                  //LOG(ERROR) << "Stealing the ResourceMsg_DataReceived: (" <<
     request_id << ") "
                  //        << delay << ", " << data_length;

                  timer_.reset(
                          new base::OneShotTimer<NzVideoProxyDispatcher>());
                  timer_->Start(
                          FROM_HERE, base::TimeDelta::FromMicroseconds(delay),
     this, &NzVideoProxyDispatcher::PostMsg);

              }
          }
      }
  */
  return handled;
}

bool NzVideoProxyDispatcher::HandleCompleteMsg(const IPC::Message& message) {
  bool handled = false;

  base::PickleIterator iter(message);
  int request_id;
  if (!iter.ReadInt(&request_id)) {
    return handled;
  }

  // LOG(ERROR) << "(" << request_id << "), ResourceMsg_RequestComplete";
  // LOG(ERROR) << "current BW: " << current_bw_;
  /*
      if (resource_dispatcher_ && active_nz_decoders_ > 0) {

          std::map<int, NzBwData*>::iterator it = bw_data_.find(request_id);
          if (it != bw_data_.end()) {
              if ((rrDelay_.size() > 0)) {

                  NzMsgDelay* md = new NzMsgDelay(request_id, base::Time(), new
     IPC::Message(message), 0); rrDelay_.push_back(md);

                  //LOG(ERROR) << "Stealing the ResourceMsg_RequestComplete: ("
     << request_id << ")";

                  handled = true;

              } else {

                  NzBwData* curr_entry = it->second;
                  bw_data_.erase(it);
                  delete curr_entry;
              }
          }
      }
  */
  return handled;
}

void NzVideoProxyDispatcher::PostMsg() {
  /*
      static int counter = 0;

      if (rrDelay_.size() > 0) {

          NzMsgDelay *md = dynamic_cast<NzMsgDelay*>(rrDelay_.front());

          if (md) {

              rrDelay_.pop_front();
              int request_id = md->request_id_;
              IPC::Message *m = md->msg_;

              //LOG(ERROR) << "Returning the Message: (" << request_id << ")
  Type: " <<
              //        (m->type() == ResourceMsg_DataReceived::ID ?
  "DataReceived" : "RequestComplete");

              std::map<int, NzBwData*>::iterator it = bw_data_.find(request_id);

              if (m->type() == ResourceMsg_RequestComplete::ID) {
                  if (it != bw_data_.end()) {
                      NzBwData* curr_entry = it->second;
                      bw_data_.erase(it);
                      delete curr_entry;
                  }
              } else {
  #ifdef CALC_APPARENT
                  // Needs rework
                  if (!md->last_time_.is_null() && (md->bits_ != 0)) {
                      base::TimeDelta time_delta = base::Time::Now() -
  md->last_time_;

                      double td = time_delta.InSecondsF();

                      if (td != 0) {
                          double rate = md->bits_ / td;
                          rate = rate * 1.0;
                          counter++;
                          if ((counter % 10) == 0)
                              LOG(ERROR) << "Apparent rate: " << rate << " td: "
  << td << " bits_ " << md->bits_;
                      }
                  }
  #else
                  counter++; // avoid unused warning
  #endif
              }

              // resource_dispatcher_->OnMessageReceived(*m);

              delete m;
              delete md;

              it->second->last_sent_time_ = base::Time::Now();

              while (rrDelay_.size() > 0) {

                  md = dynamic_cast<NzMsgDelay*>(rrDelay_.front());

                  int64 delay = calcDelay(md->bits_);

                  // If next message is not from same request, reduce delay by
                  // time since last message sent for that request
                  if (md->request_id_ != request_id) {
                      it = bw_data_.find(md->request_id_);
                      if (!it->second->last_sent_time_.is_null()) {
                          base::TimeDelta td = base::Time::Now() -
  it->second->last_sent_time_; delay -= td.InMicroseconds();
                      }
                  }

                  request_id = md->request_id_;

                  if (delay <= 0) {
                      rrDelay_.pop_front();
                      IPC::Message *m = md->msg_;

                      it = bw_data_.find(request_id);
                      if (m->type() == ResourceMsg_RequestComplete::ID) {
                          if (it != bw_data_.end()) {
                              NzBwData* curr_entry = it->second;
                              bw_data_.erase(it);
                              delete curr_entry;
                          }
                      }

                      // resource_dispatcher_->OnMessageReceived(*m);

                      delete m;
                      delete md;

                  } else {

                      timer_.reset(new
  base::OneShotTimer<NzVideoProxyDispatcher>()); timer_->Start( FROM_HERE,
  base::TimeDelta::FromMicroseconds(delay), this,
  &NzVideoProxyDispatcher::PostMsg); break;
                  }
              }
          }
      }
      */
}

int NzVideoProxyDispatcher::RenderId() {
  return render_id_;
}

void NzVideoProxyDispatcher::Create(const Nz_Proxy_Create& create_data) {
  ipc_task_runner_->PostTask(
      FROM_HERE,
      base::Bind(&NzVideoProxyDispatcher::CreateOnIOThread, this, create_data));
  reset_bw_calc_ = true;
}

void NzVideoProxyDispatcher::Start(const Nz_Proxy_Initial_Data& init_data) {
  ipc_task_runner_->PostTask(
      FROM_HERE,
      base::Bind(&NzVideoProxyDispatcher::StartOnIOThread, this, init_data));
}

void NzVideoProxyDispatcher::Update(const Nz_Proxy_Initial_Data& init_data) {
  ipc_task_runner_->PostTask(
      FROM_HERE,
      base::Bind(&NzVideoProxyDispatcher::UpdateOnIOThread, this, init_data));
}

void NzVideoProxyDispatcher::BoundingRect(
    const Nz_Proxy_Bounding_Rect& bounding_rect) {
  ipc_task_runner_->PostTask(
      FROM_HERE, base::Bind(&NzVideoProxyDispatcher::BoundingRectOnIOThread,
                            this, bounding_rect));
}

void NzVideoProxyDispatcher::Play(int id, double duration) {
  ipc_task_runner_->PostTask(
      FROM_HERE,
      base::Bind(&NzVideoProxyDispatcher::PlayOnIOThread, this, id, duration));
}

void NzVideoProxyDispatcher::Pause(const Nz_Proxy_Id& id_data) {
  ipc_task_runner_->PostTask(
      FROM_HERE,
      base::Bind(&NzVideoProxyDispatcher::PauseOnIOThread, this, id_data));
}

void NzVideoProxyDispatcher::Reset(const Nz_Proxy_Id& id_data) {
  ipc_task_runner_->PostTask(
      FROM_HERE,
      base::Bind(&NzVideoProxyDispatcher::ResetOnIOThread, this, id_data));
}

void NzVideoProxyDispatcher::Stop(const Nz_Proxy_Id& id_data) {
  ipc_task_runner_->PostTask(
      FROM_HERE,
      base::Bind(&NzVideoProxyDispatcher::StopOnIOThread, this, id_data));
}

void NzVideoProxyDispatcher::Destroy(const Nz_Proxy_Id& id_data) {
  ipc_task_runner_->PostTask(
      FROM_HERE,
      base::Bind(&NzVideoProxyDispatcher::DestroyOnIOThread, this, id_data));
}

void NzVideoProxyDispatcher::Remove(const Nz_Proxy_Id& id_data) {
  ipc_task_runner_->PostTask(
      FROM_HERE,
      base::Bind(&NzVideoProxyDispatcher::RemoveOnIOThread, this, id_data));
}

void NzVideoProxyDispatcher::Restore(const Nz_Proxy_Id& id_data) {
  ipc_task_runner_->PostTask(
      FROM_HERE,
      base::Bind(&NzVideoProxyDispatcher::RestoreOnIOThread, this, id_data));
}

void NzVideoProxyDispatcher::Buffer(const Nz_Proxy_Media_Buffer& buffer) {
  ipc_task_runner_->PostTask(
      FROM_HERE,
      base::Bind(&NzVideoProxyDispatcher::BufferOnIOThread, this, buffer));
}

void NzVideoProxyDispatcher::Hidden(const Nz_Proxy_Id& id_data) {
  ipc_task_runner_->PostTask(
      FROM_HERE,
      base::Bind(&NzVideoProxyDispatcher::HiddenOnIOThread, this, id_data));
}

void NzVideoProxyDispatcher::Shown(const Nz_Proxy_Id& id_data) {
  ipc_task_runner_->PostTask(
      FROM_HERE,
      base::Bind(&NzVideoProxyDispatcher::ShownOnIOThread, this, id_data));
}

void NzVideoProxyDispatcher::Seek(const Nz_Proxy_Seek& seek_data) {
  ipc_task_runner_->PostTask(
      FROM_HERE,
      base::Bind(&NzVideoProxyDispatcher::SeekOnIOThread, this, seek_data));
}

void NzVideoProxyDispatcher::ScrollVector(const int X, const int Y) {
  ipc_task_runner_->PostTask(
      FROM_HERE,
      base::Bind(&NzVideoProxyDispatcher::ScrollVectorOnIOThread, this, X, Y));
}

void NzVideoProxyDispatcher::CreateOnIOThread(
    const Nz_Proxy_Create& create_data) {
  LOG(ERROR) << "NzVideoProxyDispatcher::CreateOnIOThread";
  Send(new NzVideoProxyHostMsg_Create(create_data));
  active_nz_decoders_++;
}

void NzVideoProxyDispatcher::StartOnIOThread(
    const Nz_Proxy_Initial_Data& init_data) {
  LOG(INFO) << "NzVideoProxyDispatcher::StartOnIOThread";
  Send(new NzVideoProxyHostMsg_Start(init_data));
}

void NzVideoProxyDispatcher::UpdateOnIOThread(
    const Nz_Proxy_Initial_Data& init_data) {
  LOG(INFO) << "NzVideoProxyDispatcher::UpdateOnIOThread";
  Send(new NzVideoProxyHostMsg_Update(init_data));
}

void NzVideoProxyDispatcher::BoundingRectOnIOThread(
    const Nz_Proxy_Bounding_Rect& bounding_rect) {
  LOG(INFO) << "NzVideoProxyDispatcher::BoundingRectOnIOThread";
  Send(new NzVideoProxyHostMsg_BoundingRect(bounding_rect));
}

void NzVideoProxyDispatcher::PlayOnIOThread(int id, double duration) {
  LOG(INFO) << "NzVideoProxyDispatcher::PlayOnIOThread";
  Send(new NzVideoProxyHostMsg_Play(id, duration));
}

void NzVideoProxyDispatcher::PauseOnIOThread(const Nz_Proxy_Id& id_data) {
  LOG(INFO) << "NzVideoProxyDispatcher::PauseOnIOThread";
  Send(new NzVideoProxyHostMsg_Pause(id_data));
}

void NzVideoProxyDispatcher::ResetOnIOThread(const Nz_Proxy_Id& id_data) {
  LOG(INFO) << "NzVideoProxyDispatcher::ResetOnIOThread";
  Send(new NzVideoProxyHostMsg_Reset(id_data));
}

void NzVideoProxyDispatcher::StopOnIOThread(const Nz_Proxy_Id& id_data) {
  LOG(INFO) << "NzVideoProxyDispatcher::StopOnIOThread";
  Send(new NzVideoProxyHostMsg_Stop(id_data));
}

void NzVideoProxyDispatcher::DestroyOnIOThread(const Nz_Proxy_Id& id_data) {
  LOG(INFO) << "NzVideoProxyDispatcher::DestroyOnIOThread";
  Send(new NzVideoProxyHostMsg_Destroy(id_data));

  active_nz_decoders_--;
}

void NzVideoProxyDispatcher::RemoveOnIOThread(const Nz_Proxy_Id& id_data) {
  LOG(INFO) << "NzVideoProxyDispatcher::RemoveOnIOThread";
  Send(new NzVideoProxyHostMsg_Remove(id_data));
}

void NzVideoProxyDispatcher::RestoreOnIOThread(const Nz_Proxy_Id& id_data) {
  LOG(INFO) << "NzVideoProxyDispatcher::RestoreOnIOThread";
  Send(new NzVideoProxyHostMsg_Restore(id_data));
}

void NzVideoProxyDispatcher::BufferOnIOThread(
    const Nz_Proxy_Media_Buffer& buffer) {
  Send(new NzVideoProxyHostMsg_VideoBuffer(buffer));
}
void NzVideoProxyDispatcher::HiddenOnIOThread(const Nz_Proxy_Id& id_data) {
  Send(new NzVideoProxyHostMsg_Hidden(id_data));
}

void NzVideoProxyDispatcher::ShownOnIOThread(const Nz_Proxy_Id& id_data) {
  Send(new NzVideoProxyHostMsg_Shown(id_data));
}

void NzVideoProxyDispatcher::SeekOnIOThread(const Nz_Proxy_Seek& seek_data) {
  Send(new NzVideoProxyHostMsg_Seek(seek_data));
}

bool NzVideoProxyDispatcher::H264Capable() {
  return h264_capable_;
}
bool NzVideoProxyDispatcher::Vp8Capable() {
  return vp8_capable_;
}
bool NzVideoProxyDispatcher::Vp9Capable() {
  return vp9_capable_;
}

///////////////////
// Audio
///////////////////

void NzVideoProxyDispatcher::AudioCreate(const Nz_Proxy_Create& create_data) {
  ipc_task_runner_->PostTask(
      FROM_HERE, base::Bind(&NzVideoProxyDispatcher::AudioCreateOnIOThread,
                            this, create_data));
}

void NzVideoProxyDispatcher::AudioStart(
    const Nz_Audio_Initial_Data& init_data) {
  ipc_task_runner_->PostTask(
      FROM_HERE, base::Bind(&NzVideoProxyDispatcher::AudioStartOnIOThread, this,
                            init_data));
}

void NzVideoProxyDispatcher::AudioBuffer(const Nz_Proxy_Media_Buffer& buffer) {
  ipc_task_runner_->PostTask(
      FROM_HERE,
      base::Bind(&NzVideoProxyDispatcher::AudioBufferOnIOThread, this, buffer));
}

void NzVideoProxyDispatcher::AudioVolume(const Nz_Audio_Volume& volume_data) {
  ipc_task_runner_->PostTask(
      FROM_HERE, base::Bind(&NzVideoProxyDispatcher::AudioVolumeOnIOThread,
                            this, volume_data));
}

void NzVideoProxyDispatcher::AudioDestroy(const Nz_Proxy_Id& id_data) {
  ipc_task_runner_->PostTask(
      FROM_HERE, base::Bind(&NzVideoProxyDispatcher::AudioDestroyOnIOThread,
                            this, id_data));
}

void NzVideoProxyDispatcher::AudioCreateOnIOThread(
    const Nz_Proxy_Create& create_data) {
  Send(new NzVideoProxyHostMsg_AudioCreate(create_data));
}
void NzVideoProxyDispatcher::AudioStartOnIOThread(
    const Nz_Audio_Initial_Data& init_data) {
  Send(new NzVideoProxyHostMsg_AudioStart(init_data));
}
void NzVideoProxyDispatcher::AudioBufferOnIOThread(
    const Nz_Proxy_Media_Buffer& buffer) {
  Send(new NzVideoProxyHostMsg_AudioBuffer(buffer));
}
void NzVideoProxyDispatcher::AudioVolumeOnIOThread(
    const Nz_Audio_Volume& volume_data) {
  Send(new NzVideoProxyHostMsg_AudioSetVolume(volume_data));
}
void NzVideoProxyDispatcher::AudioDestroyOnIOThread(
    const Nz_Proxy_Id& id_data) {
  Send(new NzVideoProxyHostMsg_AudioDestroy(id_data));
}

///////////////////
// DRM
///////////////////

void NzVideoProxyDispatcher::CreateDecryptor(
    const Nz_Decrypt_Create& create_data) {
  ipc_task_runner_->PostTask(
      FROM_HERE, base::Bind(&NzVideoProxyDispatcher::CreateDecryptorOnIOThread,
                            this, create_data));
}

void NzVideoProxyDispatcher::GenerateKeyRequest(
    const Nz_Generate_Key_Request& request_data) {
  ipc_task_runner_->PostTask(
      FROM_HERE,
      base::Bind(&NzVideoProxyDispatcher::GenerateKeyRequestOnIOThread, this,
                 request_data));
}

void NzVideoProxyDispatcher::UpdateSession(const Nz_Key_Data& key_data) {
  ipc_task_runner_->PostTask(
      FROM_HERE, base::Bind(&NzVideoProxyDispatcher::UpdateSessionOnIOThread,
                            this, key_data));
}

void NzVideoProxyDispatcher::ReleaseSession(
    const Nz_Session_Release& session_data) {
  ipc_task_runner_->PostTask(
      FROM_HERE, base::Bind(&NzVideoProxyDispatcher::ReleaseSessionOnIOThread,
                            this, session_data));
}

void NzVideoProxyDispatcher::CreateDecryptorOnIOThread(
    const Nz_Decrypt_Create& create_data) {
  Send(new NzVideoProxyHostMsg_DecryptorCreate(create_data));
}

void NzVideoProxyDispatcher::GenerateKeyRequestOnIOThread(
    const Nz_Generate_Key_Request& request_data) {
  Send(new NzVideoProxyHostMsg_GenerateKeyRequest(request_data));
}

void NzVideoProxyDispatcher::UpdateSessionOnIOThread(
    const Nz_Key_Data& key_data) {
  Send(new NzVideoProxyHostMsg_UpdateSession(key_data));
}

void NzVideoProxyDispatcher::ReleaseSessionOnIOThread(
    const Nz_Session_Release& session_data) {
  Send(new NzVideoProxyHostMsg_ReleaseSession(session_data));
}

void NzVideoProxyDispatcher::ScrollVectorOnIOThread(const int X, const int Y) {
  Send(new NzVideoProxyHostMsg_ScrollVector(X, Y));
}

void NzVideoProxyDispatcher::NzLog(int severity, const std::string& log_msg) {
    if (ipc_task_runner_->BelongsToCurrentThread()) {
      content::ChildThreadImpl::current()->Send(new NzVideoProxyHostMsg_Log(severity, log_msg));
    }
    else {
      ipc_task_runner_->PostTask(
        FROM_HERE, base::BindOnce(&NzVideoProxyDispatcher::NzLog, this, severity, log_msg));
    }
}

void NzVideoProxyDispatcher::NzLogOnIOThread(int severity,
                                             const std::string& log_msg) {
  Send(new NzVideoProxyHostMsg_Log(severity, log_msg));
}

void NzVideoProxyDispatcher::OnDeviceProperties(bool allowUserAgentChange,
                                                bool isMobileDevice,
                                                std::string deviceSw) {
  // Note: Only static functions of NzosPlatformThread are usable in the
  // renderer
  ui::NzosPlatformThread::SetAllowUserAgentChange(allowUserAgentChange);
  ui::NzosPlatformThread::SetDeviceSw(deviceSw);
  ui::NzosPlatformThread::SetIsMobileDevice(isMobileDevice);

  LOG(INFO) << "Device Properties received: "
            << "Allow user Agent Change: "
            << ((allowUserAgentChange) ? "Yes" : "No")
            << ", Is Mobile: " << ((isMobileDevice) ? "Yes" : "No")
            << ", Device S/W: " << deviceSw;
}

double NzVideoProxyDispatcher::calcDelay(int64_t bits) {
  double des_secs = ((double)bits) / target_bw_;

  double cur_secs = ((double)bits) / current_bw_;

  int64_t delay = (des_secs - cur_secs) * base::Time::kMicrosecondsPerSecond;

  // LOG(ERROR) << "bytes: " << bits / 8 << " des_secs: " << des_secs << "
  // cur_secs: " << cur_secs << " delay: " << delay;

  return delay;
}

}  // namespace content
