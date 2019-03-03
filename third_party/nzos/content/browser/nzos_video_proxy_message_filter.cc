// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "base/command_line.h"
#include "third_party/nzos/content/browser/nzos_video_proxy_message_filter.h"
#include "content/browser/renderer_host/render_widget_host_impl.h"
#include "content/public/browser/render_process_host.h"
#include "content/public/browser/render_view_host.h"
#include "content/public/browser/render_widget_host_iterator.h"
#include "content/public/browser/web_contents.h"
// #include "content/common/resource_messages.h"
// #include "content/browser/loader/resource_message_filter.h"
#include "media/base/media_switches.h"
#include "third_party/nzos/include/NzApe.h"
#include "third_party/nzos/include/NzAppApiCb.h"
#include "third_party/nzos/include/QzProperty.h"
#include "ui/ozone/platform/nzos/nzos_platform_thread.h"
#include "ui/ozone/platform/nzos/nzos_platform_interface.h"

#include <sstream>

using namespace std;

namespace content {

// Queue size is specified in milliseconds
static int VideoQueueSize = 500;
static int AudioQueueSize = 500;
static int MaxVideoQueueSize = 5000;
static int MaxAudioQueueSize = 5000;

const char* g_KeyId = "1234567890123456";
const uint8_t* g_u8KeyId = (const uint8_t*) g_KeyId;
int g_KeyIdLen = 16;

//
// This is a interface object between ozone platform thread and message filter
// required to avoid dependency cycle in build files
// A single static instance of this class is created and passed to 
// nzos_platform_thread in the ui/ozone/platform/nzos code
//
NzVideoProxyMessageFilterPlatformInterface* NzVideoProxyMessageFilter::platformInterface_  = nullptr;

class NzVideoProxyMessageFilterPlatformInterface : public ui::NzosPlatformInterface {
  public:
    NzVideoProxyMessageFilterPlatformInterface() = default;
    ~NzVideoProxyMessageFilterPlatformInterface() = default;

    void OnDevicePropertiesReceived (void* pNzDevice) override;
    void OnKeyMessageReceived(uint32_t u32SessionId,
                              uint32_t u32KeyRqstId,
                              const uint8_t* pOpaqueData,
                              uint32_t u32OpaqueDataLen,
                              const char* url,
                              uint32_t u32UrlLen) override;

};

void NzVideoProxyMessageFilterPlatformInterface::OnDevicePropertiesReceived (void* pNzDevice) {
  LOG(ERROR) << "OnDevicePropertiesReceived";
  NzVideoProxyMessageFilter::OnDevicePropertiesReceivedS(pNzDevice);
}

void NzVideoProxyMessageFilterPlatformInterface::OnKeyMessageReceived(
  uint32_t u32SessionId,
  uint32_t u32KeyRqstId,
  const uint8_t* pOpaqueData,
  uint32_t u32OpaqueDataLen,
  const char* url,
  uint32_t u32UrlLen) {
  
    LOG(ERROR) << "OnKeyMessageReceived";
    NzVideoProxyMessageFilter::OnKeyMessageReceivedS(
      u32SessionId, 
      u32KeyRqstId, 
      pOpaqueData, 
      u32OpaqueDataLen,
      url,
      u32UrlLen);
  }


NzStream::NzStream (int rtg_id, uint32_t key_sess, int other_stream_id,
                    uint32_t drm_scheme,
                    const std::string& bypassUrl, const std::string& bypassCorr) :
    routing_id (rtg_id), other_stream_id_ (other_stream_id),
    codec_ (NZ_IPC_CODEC_UNSET), handle (NULL), pfb (NULL), video_width (0),
    video_height (0), rectSet (false), offset (0), is_started (false),
    is_shown (false), is_capturing (false), need_send_key (false),
    encrypt_video_ (true), oaes_ctx(0), key_len(0), key(0),
    key_session (key_sess), duration_ (0), drm_scheme_ (drm_scheme),
    is_playing_(false), mediaDuration_(0.0), bypassUrl_(bypassUrl),
    bypassCorr_(bypassCorr) {
 }

NzStream::~NzStream () {
  if (oaes_ctx) {
#if 0
      oaes_free(&oaes_ctx);
#endif
  }
  oaes_ctx = 0;
  if (key) {
      free(key);
  }
  key = 0;
}

NzAudioStream::NzAudioStream (int routing_id, int other_stream_id,
                              uint32_t drm_scheme, const std::string& bypassUrl,
                              const std::string& bypassCorr) :
    routing_id_ (routing_id), other_stream_id_ (other_stream_id), volume_ (0),
    drm_scheme_ (drm_scheme), paused_(false), mediaDuration_(0.0),
    bypassUrl_(bypassUrl), bypassCorr_(bypassCorr) {

  handle = NULL;
  codec_ = QZ_AUDIO_CODEC_AAC;
  rate_ = 44100;
  format_ = QZ_AUDIO_FORMAT_S16LSB;
  channels_ = 2;
}

NzAudioStream::~NzAudioStream () {
}

NzDRMSession::NzDRMSession (int id, int session, int scheme) :
    id_ (id), session_ (session), scheme_ (scheme) {

}

NzDRMSession::~NzDRMSession () {
}

static void AppVideoUrlCb(void* pSurf, uint32_t flags, double frameRate, uint64_t timestamp,
    const char* correlation, uint32_t correlationLen, const char* url, uint32_t urlLen)
{
  std::string sUrl(url, urlLen);
  LOG(ERROR) << "AppVideoUrlCb: " << sUrl << " ts: " << timestamp;
  NzSurfByPassSetUrl(pSurf, sUrl.c_str(), timestamp);
}

static void AppAudioUrlCb(void* pAudio, uint32_t flags, double frameRate, uint64_t timestamp,
    const char* correlation, uint32_t correlationLen, const char* url, uint32_t urlLen)
{
  std::string sUrl(url, urlLen);
  LOG(ERROR) << "AppAudioUrlCb: " << sUrl << " ts: " << timestamp;
  NzAudioByPassSetUrl(pAudio, sUrl.c_str(), timestamp);
}

static void AppVideoPlayCb(void* pSurf, const char* correlation, uint32_t correlationLen)
{
  LOG(ERROR) << "AppVideoPlayCb: ";
  NzSurfVideoPlay(pSurf);
}

static void AppAudioPlayCb(void* pAudio, const char* correlation, uint32_t correlationLen)
{
  LOG(ERROR) << "AppAudioPlayCb: ";
  NzAudioPlayEx2(pAudio);
}

static void AppVideoPauseCb(void* pSurf, const char* correlation, uint32_t correlationLen)
{
  LOG(ERROR) << "AppVideoPauseCb: ";
  NzSurfVideoPause(pSurf);
}

static void AppAudioPauseCb(void* pAudio, const char* correlation, uint32_t correlationLen)
{
  LOG(ERROR) << "AppAudioPauseCb: ";
  NzAudioPauseEx(pAudio);
}

static void AppVideoSeekCb(void* pSurf, const char* correlation, uint32_t correlationLen, uint64_t timestamp)
{
  LOG(ERROR) << "AppVideoSeekCb: ";
  NzSurfVideoSeek(pSurf, timestamp);
}

static void AppAudioSeekCb(void* pAudio, const char* correlation, uint32_t correlationLen, uint64_t timestamp)
{
  // Audio seek not supported by NzApe?
}

static void AppVideoSetQueueSizeCb(void* pSurf, const char* correlation, uint32_t correlationLen,
    uint64_t queueSize, uint64_t updateInterval)
{
  LOG(ERROR) << "AppVideoSetQueueSizeCb: ";
  NzSurfVideoSetQueueSize(pSurf, queueSize * 1000, updateInterval * 1000);
}

static void AppAudioSetQueueSizeCb(void* pAudio, const char* correlation, uint32_t correlationLen,
    uint64_t queueSize, uint64_t updateInterval)
{
  LOG(ERROR) << "AppAudioSetQueueSizeCb: ";
  NzAudioSetQueueSize(pAudio, queueSize * 1000, updateInterval * 1000);
}
uint32_t filts[] = { NzosMediaProxyMsgStart };

std::map<int, NzVideoProxyMessageFilter*>& NzVideoProxyMessageFilter::nz_messageFilters_ =
    *new std::map<int, NzVideoProxyMessageFilter*> ();
int NzVideoProxyMessageFilter::ids_ = 1;

//static
NzVideoProxyMessageFilter* NzVideoProxyMessageFilter::GetInstanceFromSession (
    uint32_t u32SessionId) {

  for (std::map<int, NzVideoProxyMessageFilter*>::iterator it =
      nz_messageFilters_.begin (); it != nz_messageFilters_.end (); it++) {
    NzVideoProxyMessageFilter* inst = it->second;
    map<int, NzDRMSession*>::iterator it_sess = inst->sessions_.find (
        u32SessionId);
    if (it_sess != inst->sessions_.end ()) {
      return inst;
    }
  }

  return NULL;
}

//static
bool NzVideoProxyMessageFilter::OnKeyMessageReceivedS (uint32_t u32SessionId,
                                                       uint32_t u32KeyRqstId,
                                                       const uint8_t* pOpaqueData,
                                                       uint32_t u32OpaqueDataLen,
                                                       const char* url,
                                                       uint32_t u32UrlLen) {
  bool result = false;
  NzVideoProxyMessageFilter* inst = GetInstanceFromSession (u32SessionId);
  if (inst) {
    result = true;
    inst->OnKeyMessageReceived (u32SessionId, u32KeyRqstId, pOpaqueData,
                                u32OpaqueDataLen, url, u32UrlLen);
  }
  else {
    LOG(ERROR) << "failed to find DRM Session";
  }

  return result;
}
//static
void NzVideoProxyMessageFilter::StaticInitialization() {
  NzAppApiCbs appApiCbs;
  memset(&appApiCbs, 0, sizeof(appApiCbs));

  appApiCbs.AppVideoUrlCb = AppVideoUrlCb;
  appApiCbs.AppAudioUrlCb = AppAudioUrlCb;
  appApiCbs.AppAudioPlayCb = AppAudioPlayCb;
  appApiCbs.AppVideoPlayCb = AppVideoPlayCb;
  appApiCbs.AppAudioPauseCb = AppAudioPauseCb;
  appApiCbs.AppVideoPauseCb = AppVideoPauseCb;
  appApiCbs.AppAudioSeekCb = AppAudioSeekCb;
  appApiCbs.AppVideoSeekCb = AppVideoSeekCb;
  appApiCbs.AppAudioSetQueueSizeCb = AppAudioSetQueueSizeCb;
  appApiCbs.AppVideoSetQueueSizeCb = AppVideoSetQueueSizeCb;

  RegisterAppApiCallbacks(&appApiCbs);
}

//static
void NzVideoProxyMessageFilter::OnDevicePropertiesReceivedS(void* pNzDevice) {
  for (std::map<int, NzVideoProxyMessageFilter*>::iterator it =
      nz_messageFilters_.begin(); it != nz_messageFilters_.end(); it++)  {

    NzVideoProxyMessageFilter* inst = it->second;
    inst->OnDevicePropertiesReceived(pNzDevice);
  }
}

static
int GetRenderViewId(int processId) {
  // We are updating all widgets including swapped out ones.
  std::unique_ptr<RenderWidgetHostIterator> widgets(
      RenderWidgetHostImpl::GetAllRenderWidgetHosts());
  while (RenderWidgetHost* widget = widgets->GetNextHost()) {
    RenderViewHost* rvh = RenderViewHost::From(widget);
    if (!rvh) {
      continue;
    }

    // Skip widgets in other processes.
    if (widget->GetProcess()->GetID() != processId) {
      continue;
    }

    return widget->GetRoutingID();
  }
  return 0;
}

NzVideoProxyMessageFilter::NzVideoProxyMessageFilter (
    int render_process_id) : //, ResourceMessageFilter* rmf) :
    BrowserMessageFilter (NzosMediaProxyMsgStart),
    render_process_id_ (render_process_id), //resourceMessageFilter_ (rmf),
    clearkey_encrypt_ (false), widevine_encryption_ (false) {

  LOG(ERROR) << "NzVideoProxyMessageFilter Construct: Render Process ID: "
               << render_process_id_;

  StaticInitialization();

  const base::CommandLine* cmd_line = base::CommandLine::ForCurrentProcess ();
  capture_filename_ = cmd_line->GetSwitchValueASCII (switches::kDumpNzDecoding);

  std::string param_string;
  int param_value;
  if (cmd_line->HasSwitch (switches::kNzVideoQueueSize)) {
    param_string = cmd_line->GetSwitchValueASCII (switches::kNzVideoQueueSize);
    if (!param_string.empty() && base::StringToInt(param_string, &param_value) && (param_value > 0))
    {
      LOG(ERROR) << "Setting video queue size" << param_value;
      VideoQueueSize = param_value;
    }
  }
  if (cmd_line->HasSwitch (switches::kNzAudioQueueSize)) {
    param_string = cmd_line->GetSwitchValueASCII (switches::kNzAudioQueueSize);
    if (!param_string.empty() && base::StringToInt(param_string, &param_value) && (param_value > 0))
    {
      LOG(ERROR) << "Setting audio queue size" << param_value;
      AudioQueueSize = param_value;
    }
  }
  if (cmd_line->HasSwitch (switches::kNzMaxVideoQueueSize)) {
    param_string = cmd_line->GetSwitchValueASCII (switches::kNzMaxVideoQueueSize);
    if (!param_string.empty() && base::StringToInt(param_string, &param_value) && (param_value > 0))
    {
      LOG(ERROR) << "Setting max video queue size" << param_value;
      MaxVideoQueueSize = param_value;
    }
  }
  if (cmd_line->HasSwitch (switches::kNzMaxAudioQueueSize)) {
    param_string = cmd_line->GetSwitchValueASCII (switches::kNzMaxAudioQueueSize);
    if (!param_string.empty() && base::StringToInt(param_string, &param_value) && (param_value > 0))
    {
      LOG(ERROR) << "Setting max audio queue size" << param_value;
      MaxAudioQueueSize = param_value;
    }
  }

  id_ = ids_++;
  nz_messageFilters_[id_] = this;

  if (!platformInterface_) {
    platformInterface_ = new NzVideoProxyMessageFilterPlatformInterface();
    ui::NzosPlatformThread::Instance()->SetInterface(platformInterface_);
  }

  // It appears that device properties are received before this class
  // is instantiated.
}

NzVideoProxyMessageFilter::~NzVideoProxyMessageFilter () {
  DCHECK(BrowserThread::CurrentlyOn (BrowserThread::IO));

  nz_messageFilters_.erase (id_);

  LOG(INFO) << "NzVideoProxyMessageFilter Destruct: Render Process ID: "
               << render_process_id_;

  map<int, NzStream*>::iterator it = streams_.begin ();
  while (it != streams_.end ()) {
    int id = it->first;
    NzStream* s = it->second;

    if (s && s->handle) {
      if (s->bypassCorr_.size() > 0)
      {
        AppCorrelateVideo(s->handle, NULL, 0);
      }
      NzSurfDelete (s->handle);
    }

    streams_.erase (id);
    if (s)
      delete s;
    it = streams_.begin ();
  }

  map<int, NzAudioStream*>::iterator it_audio = audioStreams_.begin ();
  while (it_audio != audioStreams_.end ()) {
    int id = it_audio->first;
    NzAudioStream* s = it_audio->second;
    if (s && s->handle) {
      if (s->bypassCorr_.size() > 0)
      {
        AppCorrelateAudio(s->handle, NULL, 0);
      }
      NzAudioDeleteEx (s->handle);
    }

    audioStreams_.erase (id);
    if (s)
      delete s;
    it_audio = audioStreams_.begin ();
  }

  map<int, NzDRMSession*>::iterator it_sess = sessions_.begin ();
  while (it_sess != sessions_.end ()) {
    int id = it_sess->first;
    NzDRMSession* s = it_sess->second;

    sessions_.erase (id);
    if (s)
      delete s;
    it_sess = sessions_.begin ();
  }

  // resourceMessageFilter_ = 0;
}

void
NzVideoProxyMessageFilter::SetEncryptionState (void* pNzDevice) {
  const base::CommandLine* cmd_line = base::CommandLine::ForCurrentProcess ();
  if (!cmd_line->HasSwitch (switches::kEnableNZDecrypting)) {
    clearkey_encrypt_ = false;
    LOG(INFO) << "Client Encryption disabled";
    return;
  }
  else {

      char strValue[128];
      NzGetDevicePropertyEx(pNzDevice, e_QzPropertyCategory_DrmScheme, 0, e_QzPropertyDrmScheme_ClearKey, strValue, sizeof(strValue));
      if (strcmp (strValue, "true") == 0) {
        clearkey_encrypt_ = true;
        LOG(INFO) << "Client Encryption will use Clear Key capability";
      }

      NzGetDevicePropertyEx(pNzDevice, e_QzPropertyCategory_DrmScheme, 0, e_QzPropertyDrmScheme_Widevine, strValue, sizeof(strValue));
      if (strcmp (strValue, "true") == 0) {
        widevine_encryption_ = true;
        LOG(INFO) << "Widevine is available";
      }
    }
}

bool
NzVideoProxyMessageFilter::OnMessageReceived (const IPC::Message& message) {
  bool handled = true;
  IPC_BEGIN_MESSAGE_MAP(NzVideoProxyMessageFilter, message)
    IPC_MESSAGE_HANDLER(NzVideoProxyHostMsg_Create, OnCreate)
    IPC_MESSAGE_HANDLER(NzVideoProxyHostMsg_Start, OnStart)
    IPC_MESSAGE_HANDLER(NzVideoProxyHostMsg_Update, OnUpdate)
    IPC_MESSAGE_HANDLER(NzVideoProxyHostMsg_BoundingRect, OnBoundingRect)
    IPC_MESSAGE_HANDLER(NzVideoProxyHostMsg_Play, OnPlay)
    IPC_MESSAGE_HANDLER(NzVideoProxyHostMsg_Pause, OnPause)
    IPC_MESSAGE_HANDLER(NzVideoProxyHostMsg_Reset, OnReset)
    IPC_MESSAGE_HANDLER(NzVideoProxyHostMsg_Stop, OnStop)
    IPC_MESSAGE_HANDLER(NzVideoProxyHostMsg_Destroy, OnDestroy)
    IPC_MESSAGE_HANDLER(NzVideoProxyHostMsg_Remove, OnRemove)
    IPC_MESSAGE_HANDLER(NzVideoProxyHostMsg_Restore, OnRestore)
    IPC_MESSAGE_HANDLER(NzVideoProxyHostMsg_VideoBuffer, OnBuffer)
    IPC_MESSAGE_HANDLER(NzVideoProxyHostMsg_Hidden, OnHidden)
    IPC_MESSAGE_HANDLER(NzVideoProxyHostMsg_Shown, OnShown)
    IPC_MESSAGE_HANDLER(NzVideoProxyHostMsg_Seek, OnSeek)

    IPC_MESSAGE_HANDLER(NzVideoProxyHostMsg_AudioCreate, OnAudioCreate)
    IPC_MESSAGE_HANDLER(NzVideoProxyHostMsg_AudioStart, OnAudioStart)
    IPC_MESSAGE_HANDLER(NzVideoProxyHostMsg_AudioBuffer, OnAudioBuffer)
    IPC_MESSAGE_HANDLER(NzVideoProxyHostMsg_AudioSetVolume, OnAudioVolume)
    IPC_MESSAGE_HANDLER(NzVideoProxyHostMsg_AudioDestroy, OnAudioDestroy)

    IPC_MESSAGE_HANDLER(NzVideoProxyHostMsg_EncryptionDetected,
                        OnEncryptionDetected)
    IPC_MESSAGE_HANDLER(NzVideoProxyHostMsg_DecryptorCreate, OnDecryptorCreate)
    IPC_MESSAGE_HANDLER(NzVideoProxyHostMsg_GenerateKeyRequest,
                        OnGenerateKeyRequest)
    IPC_MESSAGE_HANDLER(NzVideoProxyHostMsg_UpdateSession, OnUpdateSession)
    IPC_MESSAGE_HANDLER(NzVideoProxyHostMsg_ReleaseSession, OnReleaseSession)

    IPC_MESSAGE_HANDLER(NzVideoProxyHostMsg_Log, OnNzLog)
    IPC_MESSAGE_HANDLER(NzVideoProxyHostMsg_ScrollVector, OnScrollVector)

    IPC_MESSAGE_UNHANDLED(handled = false)
    IPC_END_MESSAGE_MAP()
  return handled;
}

void
NzVideoProxyMessageFilter::OnFilterAdded (IPC::Channel* sender) {
  BrowserMessageFilter::OnFilterAdded(sender);

  LOG(ERROR) << "OnFilterAdded";

  Nz_Proxy_Id id_data;
  id_data.id = render_process_id_;
  Send (new NzVideoProxyMsg_RenderID (id_data));

  // Call this now as the event arrives before this class is instantiated.
  OnDevicePropertiesReceived(NULL);

  Nz_Capabilities capabilities;
  capabilities.h264Capable = NzSurfIsCodecSupported (
  QZ_SURF_VIDEO_FEED | QZ_SURF_CODEC_H264);
  capabilities.vp8Capable = NzSurfIsCodecSupported (
  QZ_SURF_VIDEO_FEED | QZ_SURF_CODEC_VP8);
  capabilities.vp9Capable = NzSurfIsCodecSupported (
  QZ_SURF_VIDEO_FEED | QZ_SURF_CODEC_VP9);

  capabilities.clearkeyCapable = clearkey_encrypt_;
  capabilities.widevineCapable = widevine_encryption_;
  capabilities.appApiPort = NzGetAppApiPort();

  std::vector<std::string> svec;
  for (int i = 0; i < 25; i++)
  {
    char strKey[128];
    char strId[128] = {0};
    sprintf(strKey, "NzChromium.DisableById[%d]", i);
    std::string id = NzConfigGet(strKey, strId, sizeof(strId), "");
    if (id.length() > 0) {
      svec.push_back(id);
    }
  }

  capabilities.disableById = svec;

  std::vector<std::string> urlvec;
  for (int i = 0; i < 25; i++)
  {
    char strKey[128];
    char strId[128] = {0};
    sprintf(strKey, "NzChromium.DisableByUrl[%d]", i);
    std::string id = NzConfigGet(strKey, strId, sizeof(strId), "");
    if (id.length() > 0) {
      urlvec.push_back(id);
    }
  }

  capabilities.disableByUrl = urlvec;

  capabilities.detect360bycanvas = NzConfigGetBool("NzChromium.detect360bycanvas", false);

  char tmpbuff[128];
  capabilities.bypassUrl = NzConfigGet("NzChromium.bypassUrl", tmpbuff, sizeof(tmpbuff), "");
  capabilities.bypassAttr = NzConfigGet("NzChromium.bypassAttr", tmpbuff, sizeof(tmpbuff), "");

  LOG(INFO) << "Sending Capabilities: H.264("
               << (capabilities.h264Capable ? "true" : "false") << "), VP8("
               << (capabilities.vp8Capable ? "true)" : "false)") << "), VP9("
               << (capabilities.vp9Capable ? "true)" : "false)")
               << "), Clear Key("
               << (capabilities.clearkeyCapable ? "true)" : "false)")
               << "), NZ Encrypt ("
               << (capabilities.nzEncryptCapable ? "true)" : "false)")
               << "), Widevine ("
               << (capabilities.widevineCapable ? "true)" : "false)")
               << ", App Api Port = " << capabilities.appApiPort;

  Send (new NzVideoProxyMsg_Capabilities (capabilities));

  fullscreen_video_ = NzConfigGetBool("NzChromium.fullscreenvideo", true);

}

void
NzVideoProxyMessageFilter::OnDevicePropertiesReceived (void* pNzDevice) {

  LOG(INFO) << "OnDevicePropertiesReceived";
  
  SetEncryptionState (pNzDevice);

  bool useragentmod = NzConfigGetBool("NzChromium.UserAgentChange", false);

  char software[100];
  NzGetDevicePropertyEx(pNzDevice, e_QzPropertyCategory_General, 0, e_QzPropertyGeneral_Software, software, sizeof(software));

  Send (new NzVideoProxyMsg_DeviceProperties(useragentmod,
      ui::NzosPlatformThread::IsMobileDevice(),
      ui::NzosPlatformThread::GetDeviceSw()));
}

void
NzVideoProxyMessageFilter::OnKeyMessageReceived (uint32_t u32SessionId,
                                                 uint32_t u32KeyRqstId,
                                                 const uint8_t* pOpaqueData,
                                                 uint32_t u32OpaqueDataLen,
                                                 const char* url,
                                                 uint32_t u32UrlLen) {
  std::string urlStr (url, u32UrlLen);
  base::PostTaskWithTraits(
    FROM_HERE, { content::BrowserThread::UI }, 
    base::Bind(&NzVideoProxyMessageFilter::OnKeyMessageReceivedUIThread,   
    base::Unretained(this), u32SessionId, u32KeyRqstId, pOpaqueData, u32OpaqueDataLen, urlStr));
}

//
// Key message received from client through NzApe call back, send on to renderer
//
void
NzVideoProxyMessageFilter::OnKeyMessageReceivedUIThread (
    uint32_t u32SessionId, uint32_t u32KeyRqstId, const uint8_t* pOpaqueData,
    uint32_t u32OpaqueDataLen, std::string url) {

  LOG(INFO) << "OnKeyMessageReceived: " << u32SessionId << "/" << u32KeyRqstId
               << " - opaque data len: " << u32OpaqueDataLen << ", url: "
               << url.c_str ();

  if (LOG_IS_ON(INFO)) {
    const uint8_t* p = pOpaqueData;
    char buff[1000];
    char* pb = buff;
    for (uint32_t i = 0; i < u32OpaqueDataLen; i++, p++) {
      if ((*p >= ' ') && (*p <= '~')) {
        *pb++ = *p;
      }
      else {
        *pb++ = '.';
      }
      if (((i + 1) % 32) == 0) {
        *pb = 0;
        LOG(INFO) << buff;
        pb = buff;
      }
    }
    if (pb != buff) {
      *pb = 0;
      LOG(INFO) << buff;
    }
  }

  map<int, NzDRMSession*>::iterator it_sess = sessions_.find (u32SessionId);
  if (it_sess != sessions_.end ()) {
    NzDRMSession* s = it_sess->second;

    Nz_Key_Request req;
    req.id = s->id_;
    req.key_rqst_id = u32KeyRqstId;
    req.opaque_data.insert (req.opaque_data.begin (), pOpaqueData,
                            pOpaqueData + u32OpaqueDataLen);
    req.url = url;
    Send (new NzVideoProxyMsg_KeyRequest (req));

    return;
  }

  LOG(ERROR) << "Key Message received for unknown sessionId " << u32SessionId;
}

void
NzVideoProxyMessageFilter::OnCreate (const Nz_Proxy_Create& create) {
  base::PostTaskWithTraits(
    FROM_HERE, { content::BrowserThread::UI }, 
    base::Bind(&NzVideoProxyMessageFilter::OnCreateUIThread,
    base::Unretained(this), create));
}

void
NzVideoProxyMessageFilter::OnCreateUIThread (const Nz_Proxy_Create& create) {

  map<int, NzStream*>::iterator it = streams_.find (create.id);
  if (it != streams_.end ()) {
    LOG(ERROR) << "Duplicate id: " << create.id;
  }

  LOG(INFO) << "NzVideoProxyMessageFilter::OnCreate: Decoder ID: " << create.id
               << ", RenderID/RoutingID: (" << render_process_id_ << "/"
               << create.routing_id << ")" << " other stream id "
               << create.other_stream_id;

  NzStream* s = new NzStream (create.routing_id, create.id,
                              create.other_stream_id, create.decrypt_scheme,
                              create.bypass_url, create.bypass_corr);

  streams_[create.id] = s;

  if (capture_filename_.size () > 0) {
    stringstream fn;
    fn << capture_filename_ << "_" << create.routing_id << ".mp4";
    VLOG(1) << "Capturing (" << create.routing_id << ") -> "
               << fn.str ().c_str ();
    s->fs.open (fn.str ().c_str (), ios::binary);
    if (s->fs.is_open ()) {
      s->is_capturing = true;
    }
  }

}

void
NzVideoProxyMessageFilter::OnStart (const Nz_Proxy_Initial_Data& init_data) {
  base::PostTaskWithTraits(
    FROM_HERE, { content::BrowserThread::UI }, 
    base::Bind(&NzVideoProxyMessageFilter::OnStartUIThread,
    base::Unretained(this), init_data));

}

void
NzVideoProxyMessageFilter::OnStartUIThread (
    const Nz_Proxy_Initial_Data& init_data) {
  map<int, NzStream*>::iterator it = streams_.find (init_data.id);
  if (it == streams_.end () || it->second->is_started)
    return;

  NzStream* s = it->second;

  s->init_data = init_data;

  LOG(INFO) << "NzVideoProxyMessageFilter::OnStart: Decoder ID: "
               << init_data.id;

  s->codec_ = init_data.codec;
  s->video_width = init_data.video_size.width ();
  s->video_height = init_data.video_size.height ();
  s->rect.SetRect (0, 0, 0, 0);

  uint32_t surfCodec = QZ_SURF_VIDEO_FEED | QZ_SURF_CODEC_XRGB;
  const char * codecStr = "unknown";
  if (s->codec_ == NZ_IPC_CODEC_H264) {
    codecStr = "H.264";
    surfCodec |= QZ_SURF_CODEC_H264;
  }
  else if (s->codec_ == NZ_IPC_CODEC_VP8) {
    surfCodec |= QZ_SURF_CODEC_VP8;
    codecStr = "VP8";
  }
  else if (s->codec_ == NZ_IPC_CODEC_VP9) {
    surfCodec |= QZ_SURF_CODEC_VP9;
    codecStr = "VP9";
  }
  else {
    // Should not reach here
  }

  if (s->drm_scheme_ == e_QzPropertyDrmScheme_Widevine)
    surfCodec |= QZ_SURF_ENCRYPT_WIDEVINE;
  else if (s->drm_scheme_ == e_QzPropertyDrmScheme_ClearKey)
    surfCodec |= QZ_SURF_ENCRYPT_CLEARKEY;

  LOG(ERROR) << "NzSurfCreate : " << init_data.id << " other stream id "
               << s->other_stream_id_;

  LOG(ERROR) << "NzSurfCreate : Codec: " << codecStr << ", (x,y) ("
               << init_data.visible_rect.origin ().x () << ", "
               << init_data.visible_rect.origin ().y () << "), HxW: "
               << init_data.visible_rect.height () << "x"
               << init_data.visible_rect.width () << ", Video HxW: "
               << s->video_height << "x" << s->video_width;

  if (s->bypassUrl_.size() == 0) {
    s->handle = NzSurfCreate (surfCodec, s->pfb, init_data.visible_rect.width (),
                              init_data.visible_rect.height (),
                              init_data.visible_rect.origin ().x (),
                              init_data.visible_rect.origin ().y (),
                              QZ_Z_ORDER_UNDER_APP,
                              // QZ_Z_ORDER_ABOVE_APP,
                              s->video_width, s->video_height);
  }
  else {
    LOG(INFO) << "NzSurfCreate (bypass) : " << init_data.id << " URL: " <<
        s->bypassUrl_.c_str() << " correlation: " << s->bypassCorr_;

    std::string url = s->bypassUrl_.c_str();
    if (s->bypassCorr_.find_first_of("dash") == 0) {
      // If correlation ID starts with dash, then don't pass in create.
      url.empty();
    }

    s->handle = NzSurfByPassCreate(0, init_data.visible_rect.width (),
                                  init_data.visible_rect.height (),
                                  init_data.visible_rect.origin ().x (),
                                  init_data.visible_rect.origin ().y (),
                                  QZ_Z_ORDER_UNDER_APP,
                                  url.c_str(), 0);

    AppCorrelateVideo(s->handle, s->bypassCorr_.c_str(), s->bypassCorr_.size());
  }

  if (!s->handle) {
    LOG(ERROR) << "Failed to create surface.";
    return;
  }

  s->is_shown = true;
  if (fullscreen_video_) HideOthers(s);

  map<int, NzAudioStream*>::iterator ait = audioStreams_.find (
      s->other_stream_id_);

  if (ait != audioStreams_.end ()) {
    NzAudioStream* as = ait->second;

    if (as->bypassUrl_.size() == 0) {
      LOG(INFO) << "NzAudioCreateEx " << " other stream id "
                   << as->other_stream_id_ << " codec " << as->codec_ << " rate "
                   << as->rate_ << " format " << as->format_ << " channels "
                   << as->channels_;

      uint32_t frameSize = 0;
      as->handle = NzAudioCreateEx (frameSize, as->codec_, as->rate_, as->format_,
                                    as->channels_, as->drm_scheme_, true, s->handle,
                                    as->extradata_.data (),
                                    as->extradata_.size ());
    }
    else {
      LOG(INFO) << "NzAudioByPassCreate " << " other stream id "
                   << as->other_stream_id_ << " URL ";

      std::string url = as->bypassUrl_.c_str();
      if (as->bypassCorr_.find_first_of("dash") == 0) {
        // If correlation ID starts with dash, then don't pass in create.
        url.empty();
      }

      as->handle = NzAudioByPassCreate (s->handle, url.c_str(), 0);

      AppCorrelateAudio(as->handle, as->bypassCorr_.c_str(), as->bypassCorr_.size());
    }

    if (as->handle == NULL) {
      LOG(ERROR) << "Failed to create audio";
      return;
    }
  }

  if (s->need_send_key) {
    int32_t rc = NzDRMCreateRequest (e_QzPropertyDrmScheme_Netzyn, s->key_session, s->key_session, g_u8KeyId, g_KeyIdLen);
    LOG(INFO) << "NzDRMCreateRequest";
    if (rc) {
      LOG(ERROR) << "NzDRMCreateRequest failed.  rc= " << rc;
      s->encrypt_video_ = false;
    }
    else {
      s->encrypt_video_ = true;
    }
  }

  s->is_started = true;
}

void
NzVideoProxyMessageFilter::OnUpdate (const Nz_Proxy_Initial_Data& init_data) {
  base::PostTaskWithTraits(
    FROM_HERE, { content::BrowserThread::UI }, 
    base::Bind(&NzVideoProxyMessageFilter::OnUpdateUIThread,
    base::Unretained(this), init_data));
}

void
NzVideoProxyMessageFilter::OnUpdateUIThread (
    const Nz_Proxy_Initial_Data& init_data) {
  map<int, NzStream*>::iterator it = streams_.find (init_data.id);
  if (it == streams_.end () || !it->second->is_started)
    return;

  NzStream* s = it->second;

  LOG(INFO) << "NzVideoProxyMessageFilter::OnUpdate: Decoder ID: "
               << init_data.id << ", visible rect: "
               << init_data.visible_rect.ToString () << ", size: "
               << init_data.video_size.ToString ();

  s->video_width = init_data.video_size.width ();
  s->video_height = init_data.video_size.height ();

  if (s->rectSet) {
    LOG(INFO) << "NzSurfSetWindow: Decoder ID: " << init_data.id;

    if (NzSurfSetWindow (
        s->handle, s->pfb, s->rect.width (), s->rect.height (), s->rect.x (),
        s->rect.y (), (s->is_shown ? QZ_Z_ORDER_UNDER_APP : QZ_Z_ORDER_HIDDEN),
        s->video_width, s->video_height) < 0) {
      LOG(ERROR) << "NzSurfSetWindow failed.";
    }
    else {
      s->is_shown = true;
      if (fullscreen_video_) HideOthers(s);
    }
  }
}

void
NzVideoProxyMessageFilter::OnBoundingRect (const Nz_Proxy_Bounding_Rect& rect) {
  base::PostTaskWithTraits(
    FROM_HERE, { content::BrowserThread::UI }, 
    base::Bind(&NzVideoProxyMessageFilter::OnBoundingRectUIThread,
    base::Unretained(this), rect));
}

void
NzVideoProxyMessageFilter::OnBoundingRectUIThread (
    const Nz_Proxy_Bounding_Rect& rect) {

  map<int, NzStream*>::iterator it = streams_.find (rect.id);
  if (it == streams_.end () || !it->second->is_started)
    return;

  NzStream* s = it->second;
  s->rectSet = true;

  content::RenderViewHost* rvh = content::RenderViewHost::FromID (
      render_process_id_, GetRenderViewId(render_process_id_));
  if (rvh) {
    content::WebContents* wc = content::WebContents::FromRenderViewHost (rvh);
    if (wc) {
      // This just gets the offset of the main tab window.
      s->offset = wc->GetContainerBounds ().y ();
      // Kludge - appears to be 16px offset when on mobile
      if (ui::NzosPlatformThread::IsMobileDevice()) s->offset += 16;
    }
  }
  else
  {
    LOG(ERROR) << "Unable to get info on window offset: " << render_process_id_ << ", " << s->routing_id;
  }


  if ((s->rect.x () != rect.bounding_rect.x ())
      || (s->rect.y () != rect.bounding_rect.y () + s->offset)
      || (s->rect.width () != rect.bounding_rect.width ())
      || (s->rect.height () != rect.bounding_rect.height ())) {

    LOG(INFO) << "NzVideoProxyMessageFilter::OnBoundingRect Decoder ID: "
                 << rect.id << ", offset: " << s->offset << ", "
                 << rect.bounding_rect.ToString ();

    s->rect.SetRect (rect.bounding_rect.x (),
                     rect.bounding_rect.y () + s->offset,
                     rect.bounding_rect.width (), rect.bounding_rect.height ());

    if (NzSurfSetWindow (
        s->handle, s->pfb, s->rect.width (), s->rect.height (), s->rect.x (),
        s->rect.y (), (s->is_shown ? QZ_Z_ORDER_UNDER_APP : QZ_Z_ORDER_HIDDEN),
        s->video_width, s->video_height) < 0) {
      LOG(ERROR) << "NzSurfSetWindow failed.";
    }
    else {
      if (s->is_shown) {
        if (fullscreen_video_) HideOthers(s);
      }
    }
  }
}

void
NzVideoProxyMessageFilter::OnPlay (int id, double duration) {
  base::PostTaskWithTraits(
    FROM_HERE, { content::BrowserThread::UI }, 
    base::Bind(&NzVideoProxyMessageFilter::OnPlayUIThread,
    base::Unretained(this), id, duration));
}

void
NzVideoProxyMessageFilter::OnPlayUIThread (int id, double duration) {
  map<int, NzStream*>::iterator it = streams_.find (id);
  if (it != streams_.end ()) {
    LOG(INFO) << "Video OnPlay: " << id;
    NzStream* s = it->second;
    if (s) {
      if (s->mediaDuration_ != duration) {
        s->mediaDuration_ = duration;
        // Take 10 % of the duration - convert to ms
        int queueSize = (int)(duration * 100.0);
        if (queueSize > MaxVideoQueueSize) queueSize = MaxVideoQueueSize;
        if (queueSize < VideoQueueSize) queueSize = VideoQueueSize;
        LOG(ERROR) << "Setting Video Queue size: " << queueSize << "ms, (duration: " << duration << ")";
        NzSurfVideoSetQueueSize (s->handle, queueSize*1000, 100000); // convert to us
      }
      if (s->handle) {
        NzSurfVideoPlay (s->handle);
      }
      s->is_playing_ = true;
    }
  } else {
    map<int, NzAudioStream*>::iterator ait = audioStreams_.find(id);
    if (ait != audioStreams_.end()) {
      LOG(INFO) << "Audio OnPlay: " << id;
      NzAudioStream* s = ait->second;
      if (s) {
        if (s->mediaDuration_ != duration) {
          s->mediaDuration_ = duration;
          // Take 10 % of the duration - convert to ms
          int queueSize = (int)(duration * 100.0);
          if (queueSize > MaxAudioQueueSize) queueSize = MaxAudioQueueSize;
          if (queueSize < AudioQueueSize) queueSize = AudioQueueSize;
          LOG(ERROR) << "Setting Audio Queue size: " << queueSize << "ms, (duration: " << duration << ")";
          NzAudioSetQueueSize(s->handle, queueSize, 100000);
        }
        if (s->handle) {
          NzAudioPlayEx2(s->handle);
        }
        s->paused_ = false;
      }
    }
  }
}

void
NzVideoProxyMessageFilter::OnPause (const Nz_Proxy_Id& id) {
  base::PostTaskWithTraits(
    FROM_HERE, { content::BrowserThread::UI }, 
    base::Bind(&NzVideoProxyMessageFilter::OnPauseUIThread,
    base::Unretained(this), id));
}

void
NzVideoProxyMessageFilter::OnPauseUIThread (const Nz_Proxy_Id& id) {
  map<int, NzStream*>::iterator it = streams_.find (id.id);
  if (it != streams_.end ()) {
    LOG(INFO) << "Video OnPause: " << id.id;
    NzStream* s = it->second;
    if (s && s->handle) {
      NzSurfVideoPause (s->handle);
      s->is_playing_ = false;
    }
  } else {
    map<int, NzAudioStream*>::iterator ait = audioStreams_.find(id.id);
    LOG(INFO) << "Audio OnPause: " << id.id;
    NzAudioStream* s = ait->second;
    if (s && s->handle) {
      NzAudioPauseEx (s->handle);
      s->paused_ = true;
    }
  }
}

void
NzVideoProxyMessageFilter::OnReset (const Nz_Proxy_Id& id) {
  base::PostTaskWithTraits(
    FROM_HERE, { content::BrowserThread::UI }, 
    base::Bind(&NzVideoProxyMessageFilter::OnResetUIThread,
    base::Unretained(this), id));
}

void
NzVideoProxyMessageFilter::OnResetUIThread (const Nz_Proxy_Id& id) {
  map<int, NzStream*>::iterator it = streams_.find (id.id);
  if (it != streams_.end ()) {
    LOG(INFO) << "Video OnReset: " << id.id;
    NzStream* s = it->second;
    if (s && s->handle) {
      NzSurfVideoFlush (s->handle);
      s->is_playing_ = false;
    }
  } else {
    map<int, NzAudioStream*>::iterator ait = audioStreams_.find (id.id);
    if (ait != audioStreams_.end()) {
      LOG(INFO) << "Audio OnReset: " << id.id;
      NzStream* s = it->second;
      if (s && s->handle) {
        NzAudioFlushEx (s->handle);
      }   
    }
  }
}

void
NzVideoProxyMessageFilter::OnStop (const Nz_Proxy_Id& id) {
  base::PostTaskWithTraits(
    FROM_HERE, { content::BrowserThread::UI }, 
    base::Bind(&NzVideoProxyMessageFilter::OnStopUIThread,
    base::Unretained(this), id));
}

void
NzVideoProxyMessageFilter::OnStopUIThread (const Nz_Proxy_Id& id) {
  map<int, NzStream*>::iterator it = streams_.find (id.id);
  if (it == streams_.end ()) {
    LOG(ERROR) << "Failed to find matching stream: " << id.id;
    return;
  }

  LOG(INFO) << "OnStop: " << id.id;

  NzStream* s = it->second;

  if (s->handle != 0) {
    if (s->bypassCorr_.size() > 0)
    {
      AppCorrelateVideo(s->handle, NULL, 0);
    }

    NzSurfDelete (s->handle);
  }

  s->handle = 0;
  if (fullscreen_video_) ShowOther(s);
}

void
NzVideoProxyMessageFilter::OnDestroy (const Nz_Proxy_Id& id) {
  base::PostTaskWithTraits(
    FROM_HERE, { content::BrowserThread::UI }, 
    base::Bind(&NzVideoProxyMessageFilter::OnDestroyUIThread,
    base::Unretained(this), id));
}

void
NzVideoProxyMessageFilter::OnDestroyUIThread (const Nz_Proxy_Id& id) {
  map<int, NzStream*>::iterator it = streams_.find (id.id);
  if (it == streams_.end ()) {
    LOG(ERROR) << "Failed to find matching stream: " << id.id;
    return;
  }

  LOG(INFO) << "OnDestroy Video: " << id.id;

  NzStream* s = it->second;

  if (s->handle != 0) {
    if (s->bypassCorr_.size() > 0)
    {
      AppCorrelateVideo(s->handle, NULL, 0);
    }
    NzSurfDelete (s->handle);
    if (fullscreen_video_) ShowOther(s);
  }
  else {
    LOG(ERROR) << "Zero Handle";
  }

  if (s->is_capturing) {
    s->fs.close ();
  }

  streams_.erase (id.id);

  delete s;
}

void
NzVideoProxyMessageFilter::OnRemove(const Nz_Proxy_Id& id) {
  base::PostTaskWithTraits(
    FROM_HERE, { content::BrowserThread::UI }, 
    base::Bind(&NzVideoProxyMessageFilter::OnRemoveUIThread,
    base::Unretained(this), id));
}

void
NzVideoProxyMessageFilter::OnRemoveUIThread (const Nz_Proxy_Id& id) {
  map<int, NzStream*>::iterator it = streams_.find (id.id);
  if (it == streams_.end ()) {
    LOG(ERROR) << "Failed to find matching stream: " << id.id;
    return;
  }

  LOG(INFO) << "OnRemove Video: " << id.id;

  NzStream* s = it->second;

  if (s->handle != 0) {
    if (s->bypassCorr_.size() > 0)
    {
      AppCorrelateVideo(s->handle, NULL, 0);
    }
    NzSurfDelete (s->handle);
    if (fullscreen_video_) ShowOther(s);
    s->is_started = false;
    s->is_shown = false;
    s->pfb = 0;
    s->handle = 0;
  }
  else {
    LOG(ERROR) << "Zero Handle";
  }

  map<int, NzAudioStream*>::iterator ait = audioStreams_.find (
      s->other_stream_id_);

  if (ait != audioStreams_.end ()) {
    NzAudioStream* as = ait->second;

    if (as && as->handle) {
      if (as->bypassCorr_.size() > 0)
      {
        AppCorrelateAudio(as->handle, NULL, 0);
      }
      NzAudioDeleteEx (as->handle);
      as->handle = 0;
    }
  }

  if (s->is_capturing) {
    s->fs.close ();
    s->is_capturing = false;
  }
}

void
NzVideoProxyMessageFilter::OnRestore (const Nz_Proxy_Id& id) {
  base::PostTaskWithTraits(
    FROM_HERE, { content::BrowserThread::UI }, 
    base::Bind(&NzVideoProxyMessageFilter::OnRestoreUIThread,
    base::Unretained(this), id));
}

void
NzVideoProxyMessageFilter::OnRestoreUIThread (const Nz_Proxy_Id& id) {
  map<int, NzStream*>::iterator it = streams_.find (id.id);
  if (it == streams_.end ()) {
    LOG(ERROR) << "Failed to find matching stream: " << id.id;
    return;
  }

  LOG(INFO) << "OnRestore Video: " << id.id;

  NzStream* s = it->second;

  if (s->handle == NULL) {
    OnStartUIThread(s->init_data);
    if (s->is_playing_) OnPlayUIThread(id.id, s->mediaDuration_);
  }

}

void
NzVideoProxyMessageFilter::OnBuffer (const Nz_Proxy_Media_Buffer& buffer) {
  base::PostTaskWithTraits(
    FROM_HERE, { content::BrowserThread::UI }, 
    base::Bind(&NzVideoProxyMessageFilter::OnBufferUIThread,
    base::Unretained(this), buffer));
}

void
NzVideoProxyMessageFilter::OnBufferUIThread (
    const Nz_Proxy_Media_Buffer& buffer) {
  map<int, NzStream*>::iterator it = streams_.find (buffer.id);
  if (it == streams_.end () || !it->second->is_started)
    return;

  NzStream* s = it->second;

  VLOG(1) << "NzVideoProxyMessageFilter::OnBuffer: ID: " << buffer.id
               << " timestamp: " << buffer.timestamp
               << " duration: " << buffer.duration
               << (buffer.end_of_stream ? " End Of NzStream" : "");

  if (s->handle == NULL)
    return;

  if (buffer.duration != s->duration_) {
    int64_t diff = buffer.duration - s->duration_;

    if (diff > 1000 || diff < -1000) {
      VLOG(1) << "Set Duration, new: " << buffer.duration << ", old: " << s->duration_;

      NzSurfVideoSetFrameDelta (s->handle, buffer.duration);
      NzSurfVideoSetQueueSize (s->handle, VideoQueueSize*1000, 100000); // convert to us
    }

    s->duration_ = buffer.duration;
  }

  if (!buffer.end_of_stream && !buffer.buffer.empty ()) {

    uint8_t* cb = new uint8_t[buffer.buffer.size () * 2];
    uint8_t* send_ptr = cb;
    size_t send_size = buffer.buffer.size ();
    uint8_t* enc_buff = 0;

    memset (cb, 0, buffer.buffer.size () * 2);
    memcpy (cb, &buffer.buffer[0], buffer.buffer.size ());

    if (s->is_capturing) {
      for (size_t i = 0; i < buffer.buffer.size (); i++) {
        s->fs << buffer.buffer[i];
      }
    }

    uint32_t u32Flags = QZ_SURF_FLAGS_FRAME_START | QZ_SURF_FLAGS_FRAME_END;
    if (buffer.seeking)
      u32Flags |= QZ_SURF_FLAGS_FRAME_SEEK;
    if (buffer.is_key_frame)
      u32Flags |= QZ_SURF_FLAGS_FRAME_KEY;

    if (buffer.encrypted) {
      if (NzSurfSendEncData (s->handle, send_ptr, send_size, u32Flags,
                             buffer.timestamp, buffer.decrypt_scheme,
                             buffer.session_id, &buffer.key_id[0],
                             buffer.key_id.size (), &buffer.iv_data[0],
                             buffer.iv_data.size (),
                             buffer.clear_samples.size (),
                             &buffer.clear_samples[0],
                             &buffer.cipher_samples[0]) != 0) {
        LOG(ERROR) << "Failed to send data";
      }
      else {
        VLOG(1) << "Sent Clear Key encrypted Buffer to Client";
      }
    }
    else {
      if (NzSurfSendData (s->handle, send_ptr, send_size, u32Flags,
                          buffer.timestamp) != 0) {
        LOG(ERROR) << "Failed to send data";
      }
      else {
        VLOG(1) << "Sent Buffer to Client";
      }
    }
    uint32_t bw;
    if (NzGetBandwidth (bw) == 0) {
      VLOG(1) << "Bandwidth: " << bw * 1000 << "Mbps";

      Nz_Client_Bandwidth bandwidth;
      bandwidth.bandwidth = bw * 1000;

      Send (new NzVideoProxyMsg_Bandwidth (bandwidth));

    }

    delete[] cb;
    if (enc_buff)
      delete[] enc_buff;
  }
  else {
    LOG(ERROR) << "End of Stream or Empty - no data sent";
  }
}

void
NzVideoProxyMessageFilter::OnHidden (const Nz_Proxy_Id& id) {
  base::PostTaskWithTraits(
    FROM_HERE, { content::BrowserThread::UI }, 
    base::Bind(&NzVideoProxyMessageFilter::OnHiddenUIThread,
    base::Unretained(this), id));
}

void
NzVideoProxyMessageFilter::OnHiddenUIThread (const Nz_Proxy_Id& id) {
  map<int, NzStream*>::iterator it = streams_.find (id.id);
  if (it == streams_.end ())
    return;

  LOG(INFO) << "NzVideoProxyMessageFilter::OnHidden: " << id.id;

  NzStream* s = it->second;

  s->is_shown = false;

  if (NzSurfSetWindow (s->handle, s->pfb, s->rect.width (), s->rect.height (),
                       s->rect.x (), s->rect.y (),
                       QZ_Z_ORDER_HIDDEN,
                       s->video_width, s->video_height) < 0) {

    LOG(ERROR) << "NzSurfSetWindow failed.";
  }
  else {
    s->is_shown = false;
  }
}

void
NzVideoProxyMessageFilter::OnShown (const Nz_Proxy_Id& id) {
  base::PostTaskWithTraits(
    FROM_HERE, { content::BrowserThread::UI }, 
    base::Bind(&NzVideoProxyMessageFilter::OnShownUIThread,
    base::Unretained(this), id));
}

void
NzVideoProxyMessageFilter::OnShownUIThread (const Nz_Proxy_Id& id) {
  map<int, NzStream*>::iterator it = streams_.find (id.id);
  if (it == streams_.end ())
    return;

  LOG(INFO) << "NzVideoProxyMessageFilter::OnShown: " << id.id;

  NzStream* s = it->second;

  s->is_shown = true;

  if (NzSurfSetWindow (s->handle, s->pfb, s->rect.width (), s->rect.height (),
                       s->rect.x (), s->rect.y (),
                       QZ_Z_ORDER_UNDER_APP,
                       s->video_width, s->video_height) < 0) {
    LOG(ERROR) << "NzSurfSetWindow failed.";
  }
  else
  {
    s->is_shown = true;
    if (fullscreen_video_) HideOthers(s);
  }
}

void NzVideoProxyMessageFilter::OnSeek(const Nz_Proxy_Seek& seek_data)
{
    base::PostTaskWithTraits(
    FROM_HERE, { content::BrowserThread::UI }, 
      base::Bind(&NzVideoProxyMessageFilter::OnSeekUIThread,
      base::Unretained(this), seek_data));
}

void NzVideoProxyMessageFilter::OnSeekUIThread(const Nz_Proxy_Seek& seek_data)
{
    map<int, NzStream*>::iterator it = streams_.find(seek_data.id);
    if (it == streams_.end())
        return;

    LOG(INFO) << "NzVideoProxyMessageFilter::OnSeek: " << seek_data.id;

    NzStream* s = it->second;

    // For bypass don't seek here.
    if (s->bypassUrl_.size() > 0) return;

    if (NzSurfVideoSeek(s->handle, seek_data.seek_time) < 0) {
        LOG(ERROR) << "NzSurfVideoSeek failed.";
    }
}

void
NzVideoProxyMessageFilter::OnAudioCreate (const Nz_Proxy_Create& create) {
  base::PostTaskWithTraits(
    FROM_HERE, { content::BrowserThread::UI }, 
    base::Bind(&NzVideoProxyMessageFilter::OnAudioCreateUIThread,
    base::Unretained(this), create));
}

void
NzVideoProxyMessageFilter::OnAudioCreateUIThread (
    const Nz_Proxy_Create& create) {

  map<int, NzAudioStream*>::iterator it = audioStreams_.find (create.id);
  if (it != audioStreams_.end ()) {
    LOG(ERROR) << "Duplicate id: " << create.id;
    // Destroy old audio object.
    Nz_Proxy_Id id;
    id.id = create.id;
    OnAudioDestroyUIThread(id);
  }

  LOG(INFO) << "NzVideoProxyMessageFilter::OnAudioCreate: Decoder ID: "
               << create.id << ", RenderID/RoutingID: (" << render_process_id_
               << "/" << create.routing_id << ")" << " other stream id "
               << create.other_stream_id;

  NzAudioStream* s = new NzAudioStream (create.routing_id,
                                        create.other_stream_id,
                                        create.decrypt_scheme,
                                        create.bypass_url,
                                        create.bypass_corr);
  s->codec_ = create.codec;
  s->rate_ = create.rate;
  s->format_ = create.format;
  s->channels_ = create.channels;
  s->extradata_.insert (s->extradata_.begin (), create.extradata.begin (),
                        create.extradata.end ());
  audioStreams_[create.id] = s;

  LOG(INFO) << "OnAudioCreate";
}

void
NzVideoProxyMessageFilter::OnAudioStart (
    const Nz_Audio_Initial_Data& init_data) {
  base::PostTaskWithTraits(
    FROM_HERE, { content::BrowserThread::UI }, 
    base::Bind(&NzVideoProxyMessageFilter::OnAudioStartUIThread,
    base::Unretained(this), init_data));
}
void
NzVideoProxyMessageFilter::OnAudioStartUIThread (
    const Nz_Audio_Initial_Data& init_data) {

  map<int, NzAudioStream*>::iterator it = audioStreams_.find (init_data.id);
  if (it == audioStreams_.end ())
    return;

  NzAudioStream* s = it->second;

  if (s->handle)
    // Audio already created
    return;

  map<int, NzStream*>::iterator vit = streams_.find (s->other_stream_id_);

  void* vhandle = NULL;
  if (vit != streams_.end ()) {
    vhandle = (vit->second)->handle;
    if (vhandle == NULL) {
      // hold off on audio create until surface created
      LOG(INFO) << "OnAudioStart: create audio deferred to surf create";
      return;
    }
  }


  uint32_t frameSize = 0;
  if (s->bypassUrl_.size() == 0) {
    LOG(INFO) << "NzAudioCreateEx " << " other stream id "
                 << s->other_stream_id_ << " codec " << s->codec_ << " rate "
                 << s->rate_ << " format " << s->format_ << " channels "
                 << s->channels_;
    s->handle = NzAudioCreateEx (frameSize, s->codec_, s->rate_, s->format_,
                                 s->channels_, s->drm_scheme_, true, vhandle,
                                 s->extradata_.data (), s->extradata_.size ());
  }
  else {
    LOG(INFO) << "Create Audio By Pass, URL: " << s->bypassUrl_;
    s->handle = NzAudioByPassCreate (s->handle, s->bypassUrl_.c_str(), 0);

    AppCorrelateAudio(s->handle, s->bypassCorr_.c_str(), s->bypassCorr_.size());
  }

  if (s->handle == NULL) {
    LOG(ERROR) << "Failed to create audio";
    return;
  }

  if (!vhandle) {
    LOG(INFO) << "Standalone audio, set queue size";
    NzAudioSetQueueSize(s->handle, AudioQueueSize, 100000);
  }

  LOG(INFO) << "OnAudioStart: ";
}

void
NzVideoProxyMessageFilter::OnAudioBuffer (const Nz_Proxy_Media_Buffer& buffer) {
  base::PostTaskWithTraits(
    FROM_HERE, { content::BrowserThread::UI }, 
    base::Bind(&NzVideoProxyMessageFilter::OnAudioBufferUIThread,
    base::Unretained(this), buffer));
}
void
NzVideoProxyMessageFilter::OnAudioBufferUIThread (
    const Nz_Proxy_Media_Buffer& buffer) {
  map<int, NzAudioStream*>::iterator it = audioStreams_.find (buffer.id);
  if (it == audioStreams_.end ())
    return;

  NzAudioStream* s = it->second;

  if (s->handle == NULL)
    // handle not created until surface is created.
    return;

  VLOG(1) << "NzVideoProxyMessageFilter::OnAudioBuffer: ID: " << buffer.id
               << " timestamp: " << buffer.timestamp;

  if (s->volume_ > 0) {
    NzAudioSetVolumeEx (s->handle, (uint32_t) (s->volume_));
    s->volume_ = 0;
  }

  // Passthru audio immediately returns.

  if (buffer.encrypted) {
    VLOG(1) << "OnAudioBuffer Encrypted, buffer size: "
               << buffer.buffer.size ();

    int rc = NzAudioPlayEncEx (s->handle, (uint8_t*) &buffer.buffer[0],
                               buffer.buffer.size (), buffer.timestamp, 0, 
                               buffer.decrypt_scheme, &buffer.key_id[0],
                               buffer.key_id.size (), &buffer.iv_data[0],
                               buffer.iv_data.size (),
                               buffer.clear_samples.size (),
                               &buffer.clear_samples[0],
                               &buffer.cipher_samples[0]);
    if (rc < 0) {
      LOG(ERROR) << "Failed to play audio: " << rc;
      return;
    }
  }
  else if (!s->paused_){

    //const uint8_t* buff = &buffer.buffer[7];
    VLOG(1) << "OnAudioBuffer, " << buffer.buffer.size () << " timestamp: "
               << buffer.timestamp;
    int rc = NzAudioPlayEx (s->handle, (uint8_t*) &buffer.buffer[0],
                            buffer.buffer.size (), buffer.timestamp, 0);
    if (rc < 0) {
      LOG(ERROR) << "Failed to play audio: " << rc;
      return;
    }

    if (fsaudio.is_open ()) {
      for (size_t i = 0; i < buffer.buffer.size (); i++) {
        fsaudio << buffer.buffer[i];
      }
    }
  }
}

void
NzVideoProxyMessageFilter::OnAudioVolume (const Nz_Audio_Volume& volume_data) {
  base::PostTaskWithTraits(
    FROM_HERE, { content::BrowserThread::UI }, 
    base::Bind(&NzVideoProxyMessageFilter::OnAudioVolumeUIThread,
    base::Unretained(this), volume_data));
}
void
NzVideoProxyMessageFilter::OnAudioVolumeUIThread (
    const Nz_Audio_Volume& volume_data) {
  map<int, NzAudioStream*>::iterator it = audioStreams_.find (volume_data.id);
  if (it == audioStreams_.end ())
    return;

  NzAudioStream* s = it->second;

  s->volume_ = volume_data.volume * 128;

  LOG(INFO) << "Volume: " << volume_data.volume;

  if (s->handle == NULL) {
    s->volume_ = volume_data.volume * 128;
    return;
  }

  NzAudioSetVolumeEx (s->handle, (uint32_t) (volume_data.volume * 128));
}

void
NzVideoProxyMessageFilter::OnAudioDestroy (const Nz_Proxy_Id& id_data) {
  base::PostTaskWithTraits(
    FROM_HERE, { content::BrowserThread::UI }, 
    base::Bind(&NzVideoProxyMessageFilter::OnAudioDestroyUIThread,
    base::Unretained(this), id_data));
}
void
NzVideoProxyMessageFilter::OnAudioDestroyUIThread (const Nz_Proxy_Id& id_data) {
  LOG(INFO) << "OnAudioDestroy";

  map<int, NzAudioStream*>::iterator it = audioStreams_.find (id_data.id);
  if (it == audioStreams_.end ())
    return;

  NzAudioStream* s = it->second;

  if (s && s->handle) {
    if (s->bypassCorr_.size() > 0)
    {
      AppCorrelateAudio(s->handle, NULL, 0);
    }
    NzAudioDeleteEx (s->handle);
  }

  audioStreams_.erase (id_data.id);

  fsaudio.close ();
}

void
NzVideoProxyMessageFilter::OnEncryptionDetected (const Nz_Proxy_Id& id_data) {
  base::PostTaskWithTraits(
    FROM_HERE, { content::BrowserThread::UI }, 
    base::Bind(&NzVideoProxyMessageFilter::OnEncryptionDetectedUIThread,
    base::Unretained(this), id_data));
}

void
NzVideoProxyMessageFilter::OnEncryptionDetectedUIThread (
    const Nz_Proxy_Id& id_data) {
  map<int, NzStream*>::iterator it = streams_.find (id_data.id);
  if (it == streams_.end ())
    return;

//  NzStream* s = it->second;

}

void
NzVideoProxyMessageFilter::OnDecryptorCreate (
    const Nz_Decrypt_Create& create_data) {
  base::PostTaskWithTraits(
    FROM_HERE, { content::BrowserThread::UI }, 
    base::Bind(&NzVideoProxyMessageFilter::OnDecryptorCreateUIThread,
    base::Unretained(this), create_data));
}

void
NzVideoProxyMessageFilter::OnDecryptorCreateUIThread (
    const Nz_Decrypt_Create& create_data) {
  map<int, NzDRMSession*>::iterator it = sessions_.find (create_data.id);
  if (it != sessions_.end ()) {
    LOG(ERROR) << "Duplicate Session";
    NzDRMSession* s = it->second;
    if (s)
      delete s;
    sessions_.erase (create_data.id);
  }

  NzDRMSession* s = new NzDRMSession (create_data.id, create_data.id,
                                      create_data.scheme);

  sessions_[create_data.id] = s;

  LOG(INFO) << "OnDecryptorCreate: " << create_data.id << "/"
               << create_data.scheme;

}

void
NzVideoProxyMessageFilter::OnGenerateKeyRequest (
    const Nz_Generate_Key_Request& request_data) {
  base::PostTaskWithTraits(
    FROM_HERE, { content::BrowserThread::UI }, 
    base::Bind(&NzVideoProxyMessageFilter::OnGenerateKeyRequestUIThread,
    base::Unretained(this), request_data));
}

void
NzVideoProxyMessageFilter::OnGenerateKeyRequestUIThread (
    const Nz_Generate_Key_Request& request_data) {
  map<int, NzDRMSession*>::iterator it = sessions_.find (request_data.id);
  if (it == sessions_.end ()) {
    LOG(ERROR) << "OnUpdateSession session not found: " << request_data.id;
    return;
  }

  LOG(INFO) << "NzDRMCreateRequest: Session Id: " << request_data.id
               << ", Key Request Id: " << request_data.key_rqst_id;

  NzDRMCreateRequest (request_data.scheme, request_data.id,
                      request_data.key_rqst_id, &request_data.init_data[0],
                      request_data.init_data.size ());
}

void
NzVideoProxyMessageFilter::OnUpdateSession (const Nz_Key_Data& key_data) {
  base::PostTaskWithTraits(
    FROM_HERE, { content::BrowserThread::UI }, 
    base::Bind(&NzVideoProxyMessageFilter::OnUpdateSessionUIThread,
    base::Unretained(this), key_data));
}

void
NzVideoProxyMessageFilter::OnUpdateSessionUIThread (
    const Nz_Key_Data& key_data) {

  map<int, NzDRMSession*>::iterator it = sessions_.find (key_data.id);
  if (it == sessions_.end ()) {
    LOG(ERROR) << "OnUpdateSession session not found: " << key_data.id;
    return;
  }

  NzDRMSession* s = it->second;

  LOG(INFO) << "NzDRMSetKey: Session Id: " << key_data.id
               << ", init data size: " << key_data.init_data.size ();
  if (key_data.init_data.size () == 0) {
    NzDRMSetKey (s->scheme_, key_data.id, key_data.key_rqst_id, NULL, 0,
                 &key_data.key_data[0], key_data.key_data.size ());
  }
  else {
    NzDRMSetKey (s->scheme_, key_data.id, key_data.key_rqst_id,
                 &key_data.init_data[0], key_data.init_data.size (),
                 &key_data.key_data[0], key_data.key_data.size ());
  }

}

void
NzVideoProxyMessageFilter::OnReleaseSession (
    const Nz_Session_Release& session_data) {
  base::PostTaskWithTraits(
    FROM_HERE, { content::BrowserThread::UI }, 
    base::Bind(&NzVideoProxyMessageFilter::OnReleaseSessionUIThread,
    base::Unretained(this), session_data));
}

void
NzVideoProxyMessageFilter::OnReleaseSessionUIThread (
    const Nz_Session_Release& session_data) {

  map<int, NzDRMSession*>::iterator it = sessions_.find (session_data.id);
  if (it == sessions_.end ()) {
    LOG(ERROR) << "OnReleaseSession session not found: " << session_data.id;
    return;
  }

  NzDRMSession* s = it->second;

  NzDRMRelease (s->scheme_, session_data.id);

  delete s;

  sessions_.erase (session_data.id);

  LOG(INFO) << "OnReleaseSession: " << session_data.id;
}

void
NzVideoProxyMessageFilter::OnNzLog (int severity, const std::string& log_msg) {
  if (severity <= ::logging::LOG_VERBOSE)
  {
    NzLogVerbose("%s", log_msg.c_str ());
  }
  else
  {
    NzLog ("%s", log_msg.c_str ());
  }
}

void NzVideoProxyMessageFilter::OnScrollVector(int X, int Y) {
  ui::NzosPlatformThread::Instance()->SetScrollVector(X, Y);
}

void NzVideoProxyMessageFilter::HideOthers(const NzStream* s)
{
  LOG(INFO) << "HideOthers: " << s->key_session;
  map<int, NzStream*>::iterator it = streams_.begin();
  while (it != streams_.end ())
  {
    NzStream* sit = it->second;
    if ((sit->key_session != s->key_session) && (sit->is_shown))
    {
      NzSurfSetWindow (
          sit->handle, sit->pfb, sit->rect.width (), sit->rect.height (), sit->rect.x (),
          sit->rect.y (), QZ_Z_ORDER_HIDDEN,
          sit->video_width, sit->video_height);
      sit->is_shown = false;
      LOG(INFO) << "Hiding: " << sit->key_session;

      map<int, NzAudioStream*>::iterator ait = audioStreams_.find (
          sit->other_stream_id_);

      if (ait != audioStreams_.end ()) {
        NzAudioStream* aStream = ait->second;

        if (aStream && aStream->handle) {
          NzAudioPauseEx (aStream->handle);
          aStream->paused_ = true;
        }
      }
    }

    it++;
  }
}

void NzVideoProxyMessageFilter::ShowOther(const NzStream* s)
{
  LOG(INFO) << "ShowOther: " << s->key_session;

  map<int, NzStream*>::iterator it = streams_.begin();
  NzStream* theStream = NULL;

  while (it != streams_.end ())
  {
    NzStream* sit = it->second;
    if (sit->key_session != s->key_session)
    {
      if (!theStream)
      {
        theStream = sit;
      }
      else if (sit->key_session > theStream->key_session)
      {
        theStream = sit;
      }
    }

    it++;
  }

  if (theStream)
  {
    LOG(INFO) << "Showing: " << theStream->key_session;

    NzSurfSetWindow (
        theStream->handle, theStream->pfb, theStream->rect.width (),
        theStream->rect.height(), theStream->rect.x (),
        theStream->rect.y (), QZ_Z_ORDER_UNDER_APP,
        theStream->video_width, theStream->video_height);
    theStream->is_shown = true;

    map<int, NzAudioStream*>::iterator ait = audioStreams_.find (
        theStream->other_stream_id_);

    if (ait != audioStreams_.end ()) {
      NzAudioStream* aStream = ait->second;

      if (aStream) {
        aStream->paused_ = false;
      }
    }
  }

}

}  // namespace content

