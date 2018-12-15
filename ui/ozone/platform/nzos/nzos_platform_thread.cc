/*
*  nzos_platform_thread.cc
*
*  Copyright (c) 2012 Netzyn Inc. All rights reserved.
*  Use of this source code maybe governed by a BSD-style license in the future.
*  Portions of this source code maybe covered by certain patents.
*
*  NzOS Application Platform Environment
*
*/

#include "time.h"
#include "base/base_switches.h"
#include "base/bind.h"
#include "base/files/file_path.h"
#include "base/command_line.h"
#include "base/logging.h"
#include "base/threading/thread.h"
#include "base/task/post_task.h"
#include "content/public/browser/browser_task_traits.h"
// #include "chrome/browser/ui/browser.h"
#include "content/browser/browser_main_loop.h"
// #include "content/shell/browser/shell.h"
// #include "content/shell/browser/shell_platform_data_aura.h"
#include "third_party/skia/include/core/SkBitmap.h"
#include "ui/base/clipboard/clipboard.h"
#include "ui/base/clipboard/clipboard_aura.h"
#include "ui/display/manager/display_manager.h"
#include "ui/display/manager/managed_display_info.h"

#if defined(OS_CHROMEOS)
#include "chrome/browser/lifetime/application_lifetime.h"
// #include "ash/display/display_info.h"
// #include "ash/display/display_manager.h"
#include "ash/shell.h"
#endif

#include "ui/ozone/platform/nzos/nzos_cursor_factory.h"
#include "ui/ozone/platform/nzos/nzos_location_provider.h"
#include "ui/ozone/platform/nzos/nzos_platform_thread.h"
#include "ui/ozone/platform/nzos/nzos_event_factory.h"
// #include "nzos/audio/nzos_audio_manager.h"
#include "third_party/nzos/include/QzProperty.h"
#include "third_party/nzos/include/QzMouse.h"
#include "third_party/nzos/include/QzKeys.h"
#include "third_party/nzos/include/NzApe.h"
// #include "third_party/nzos/content/browser/video_proxy/nz_video_proxy_message_filter.h"
// #include "nzos/video_capture/video_capture_device_nzos.h"

uint32_t g_Width       = 0;
uint32_t g_Height      = 0;
uint32_t g_Stride      = 0;
int      g_DevDpi      = 96;
float    g_UIScale     = 1.0f;
float    g_DeviceScale = 1.0f;

namespace content {

void ImmediateShutdownAndExitProcess();
}

namespace ui {

// Copied from clipboard_aura.cc
enum AuraClipboardFormat {
  TEXT      = 1 << 0,
  HTML      = 1 << 1,
  RTF       = 1 << 2,
  BOOKMARK  = 1 << 3,
  BITMAP    = 1 << 4,
  CUSTOM    = 1 << 5,
  WEB       = 1 << 6,
};

#pragma pack(2)
typedef struct {
  uint16_t type;
  uint32_t size;
  uint16_t  reserved1;
  uint16_t  reserved2;
  uint32_t offsetBits;
} tBMPFileHeader;

typedef struct {
  uint32_t size;
  uint32_t width;
  uint32_t height;
  uint16_t planes;
  uint16_t bitCount;
  uint32_t compression;
  uint32_t sizeImage;
  uint32_t xPelsPerMeter;
  uint32_t yPelsPerMeter;
  uint32_t clrUsed;
  uint32_t clrImportant;
} tBMPInfoHeader;

typedef struct {
  uint32_t red;
  uint32_t green;
  uint32_t blue;
} tRGBQuads;
#pragma pack()

static void DumpHex(const uint8_t* buffer, int count)
{
    char buff[1024];
    char* ptr = buff;
    bool empty = true;
    for (int i = 0; i < count; i++)
    {
        ptr += sprintf(ptr, "0x%2.2X, ", buffer[i]);
        empty = false;
        if ((i + 1) % 16 == 0)
        {
            *ptr = 0;
            NzLogVerbose("%s", buff);
            ptr = buff;
            empty = true;
        }
    }
    if (!empty) NzLogVerbose("%s", buff);
}

static uint32_t ChromeToNzFormats(uint32_t formats) {
  uint32_t nzFormats = 0;
  if ((formats & TEXT) == TEXT) nzFormats |= QZ_CLIPBOARD_FORMAT_TEXT;
  // TODO - add HTML, BOOKMARK as text?
  if ((formats & BITMAP) == BITMAP) nzFormats |= QZ_CLIPBOARD_FORMAT_IMAGE;

  return nzFormats;
}

//TODOSJ
// static uint32_t NzToChromeFormat(uint32_t formats) {
//   uint32_t chromeFormats = 0;
//   if ((formats & QZ_CLIPBOARD_FORMAT_TEXT) == QZ_CLIPBOARD_FORMAT_TEXT) chromeFormats |= TEXT;
//   // TODO - add HTML, BOOKMARK as text?
// //  if ((formats & QZ_CLIPBOARD_FORMAT_IMAGE) == QZ_CLIPBOARD_FORMAT_IMAGE) nchromeFormats |= BITMAP;

//   return chromeFormats;
// }

bool NzLogMessageHandlerFunction(int severity, const char* file, int line, size_t message_start, const std::string& str) {
  if (str.length() > 1000) {
    std::string repstr = str.substr(0, 999);
    repstr += ". . . truncated";
    if (severity <= ::logging::LOG_VERBOSE) {
      NzLogVerbose("%s", repstr.c_str());
    } else {
      NzLog("%s", repstr.c_str());
    }
  }
  else {
    if (severity <= ::logging::LOG_VERBOSE) {
      NzLogVerbose("%s", str.c_str());
    } else {
      NzLog("%s", str.c_str());
    }
  }
  return true;
}

std::string NzosPlatformThread::sDeviceType;
std::string NzosPlatformThread::sDeviceSw;
bool NzosPlatformThread::sAllowUserAgentChange = false;
bool NzosPlatformThread::sIsMobileDevice = false;

NzosPlatformThread::NzosPlatformThread() : Thread("NzosPlatformThread"), state_(NzosPlatformState::Startup),
    app_has_focus_(false), weak_factory_(this) {
}

NzosPlatformThread::~NzosPlatformThread() { 
  Stop(); 
  if (!IsShutdown()) {
    // Shutdown NzApe
    NzLog("Platform thread shutdown");
    NzosPlatformThread::Instance()->SetState(NzosPlatformState::Shutdown);
  }
  locationProvider_ = nullptr;
}

NzosPlatformThread* NzosPlatformThread::Instance() {
  static NzosPlatformThread* instance_ = NULL;
  if (!instance_) 
    instance_ = new NzosPlatformThread();

  return instance_;
}

bool NzosPlatformThread::IsAppConnected() {
  return (NzosEventFactory::GetInstance() && NzosEventFactory::GetInstance()->EventConverter() &&
          ((state_ == NzosPlatformState::AppConnected) || (state_ == NzosPlatformState::AppVisible)));
}

bool NzosPlatformThread::IsAppVisible() {
  return (NzosEventFactory::GetInstance() && NzosEventFactory::GetInstance()->EventConverter() &&
          (state_ == NzosPlatformState::AppVisible));
}

void NzosPlatformThread::EventConnected(const char* pstrURL) {
  uint32_t* fb;
  uint32_t encoder_width, encoder_height, flags;
  NzGetDisplayInfoEx(fb, g_Width, g_Height, g_Stride, encoder_width, encoder_height, flags);

  NzLog("0x%x, %dx%d - %d", fb, g_Width, g_Height, g_Stride);

  uint32_t scale = NzConfigGetInt("NzApp.UIScale", 100);
  g_UIScale = (scale * 1.0) / 100;

  g_DeviceScale = CalculateDeviceScale();

  sAllowUserAgentChange = NzConfigGetBool("NzChromium.UserAgentChange", false);

  NzLog("Device connected! URL=%s Window=%dx%d UIScale:%f DeviceScale:%f", pstrURL, g_Width, g_Height, g_UIScale, g_DeviceScale);

  std::stringstream ss;
  ss << g_Width << "x" << g_Height << "*" << g_DeviceScale;
  base::CommandLine::ForCurrentProcess()->AppendSwitchASCII("--ash-host-window-bounds", ss.str());
  NzLog("Set NzChromium window to be required size: %s", ss.str().c_str());

  NzosPlatformThread::Instance()->SetState(NzosPlatformState::DeviceConnected);
  NzSetCursor(QZ_CURSOR_ARROW);
}

void NzosPlatformThread::EventDisconnected() {
  NzLog("Device disconnected!");
  NzosPlatformThread::Instance()->SetState(NzosPlatformState::DeviceDisconnected);
  if (NzosPlatformThread::Instance()->IsAppConnected())
    NzosEventFactory::GetInstance()->EventConverter()->CloseWidget(1); // TODO get the widget handle and pass it in here
}

void NzosPlatformThread::EventDevicePropertiesEx(void* pNzDevice) {
  NzLog("Event Device Properties");

  g_DevDpi = 96;
  char sDpi[100];
  if (NzGetDevicePropertyEx(pNzDevice, e_QzPropertyCategory_Display, 0, e_QzPropertyDisplay_Dpi, sDpi, (uint32_t)sizeof(sDpi)) == 0) {
    int dpi = strtol(sDpi, NULL, 10);
    if (dpi > 0) {
      g_DevDpi = dpi;
    }
  }

  char deviceType[100];
  NzGetDevicePropertyEx(pNzDevice, e_QzPropertyCategory_General, 0, e_QzPropertyGeneral_DeviceType, deviceType, sizeof(deviceType));
  sDeviceType = deviceType;

  uint32_t devtype = NzGetDeviceType(pNzDevice);
  if ((devtype == QZ_DEVICE_TYPE_PHONE) || (devtype == QZ_DEVICE_TYPE_TABLET)) {
    sIsMobileDevice = true;
  }
  else
  {
    sIsMobileDevice = false;
  }

  char pstrKey[100];
  sprintf(pstrKey, "NzChromium.DeviceSW[%s]", deviceType);

  char software[100];
  NzConfigGet(pstrKey, software, 100, "");

  sDeviceSw = software;

  if (Instance()->platformInterface_) Instance()->platformInterface_->OnDevicePropertiesReceived(pNzDevice);
}

void NzosPlatformThread::EventKeyboard(uint32_t Op, uint32_t u32Flags, uint32_t Key) {
  if (NzosPlatformThread::Instance()->IsAppConnected()) {
    if (NzosEventFactory::GetInstance()->EventConverter()) {
      NzosEventFactory::GetInstance()->EventConverter()->KeyNotify(Op, u32Flags, Key);
    }
    else {
      LOG(ERROR) << "No event converter set";
    }
  }
}

void NzosPlatformThread::EventJoystick(uint32_t u32JoystickId, uint32_t u32AxisCount, uint32_t u32AxisInput, const int32_t* i32AxisValues, uint32_t u32ButtonCount, uint64_t u64ButtonInput, uint64_t u64ButtonStates) {
  // Not implemented
}

void NzosPlatformThread::EventDRMKeyRequest(uint32_t u32SessionId, uint32_t u32KeyRqstId, const uint8_t* pOpaqueData, uint32_t u32OpaqueDataLen, const char* pUrl, uint32_t u32UrlLen) {
  if (Instance()->platformInterface_) Instance()->platformInterface_->OnKeyMessageReceived(u32SessionId, u32KeyRqstId, pOpaqueData, u32OpaqueDataLen, pUrl, u32UrlLen);
}

void NzosPlatformThread::EventMouse(uint32_t Op, uint32_t u32Flags, uint32_t X, uint32_t Y) {
  if (NzosPlatformThread::Instance()->IsAppConnected()) {
    if (NzosEventFactory::GetInstance()->EventConverter()) {
      NzosEventFactory::GetInstance()->EventConverter()->ButtonNotify(Op, u32Flags, X, Y);
    }
    else {
      LOG(ERROR) << "No event converter set";
    }
  }
}

void NzosPlatformThread::EventTouch(uint64_t u64EventTime, uint32_t u32TouchId, uint32_t u32Flags, uint32_t u32Op, uint32_t u32FingerId, uint32_t X, uint32_t Y, double dPressure, uint32_t u32TapFingers) {
  if (NzosPlatformThread::Instance()->IsAppConnected())
    NzosEventFactory::GetInstance()->EventConverter()->TouchNotify(u64EventTime, u32Op, u32Flags, u32FingerId, X, Y);
}

void NzosPlatformThread::EventVisible(bool bVisible) {
  NzLog("Event Visible,%svisible", (bVisible ? " " : " not "));

  if (bVisible) {
    NzosPlatformThread::Instance()->SetState(NzosPlatformState::AppVisible);
    const base::CommandLine* cmd_line = base::CommandLine::ForCurrentProcess();
    if (!cmd_line->HasSwitch(switches::kNzInhibitVisibleAck)) {
      NzVisibleAck();
    }
  } else {
    NzosPlatformThread::Instance()->SetState(NzosPlatformState::AppConnected);
  }

  NzSetDisplaySize(g_Width, g_Height, g_Stride);
}

void NzosPlatformThread::EventResized(std::vector<display::ManagedDisplayInfo> displays) {
  NzLog("NzosPlatformThread::EventResized");
  //TODOSJ
  // ash::Shell::Get()->display_manager()->OnNativeDisplaysChanged(displays);
  NzSetDisplaySize(g_Width, g_Height, g_Stride);
}

void NzosPlatformThread::EventResize(uint32_t WindowWidth, uint32_t WindowHeight) {
  NzLog("EventResize - client requesting app surface size change: %dx%d", WindowWidth, WindowHeight);
  g_Width = WindowWidth;
  g_Height = WindowHeight;
  g_Stride = WindowWidth;

#if defined(OS_CHROMEOS)
  std::stringstream ss;
  ss << g_Width << "x" << g_Height << "*" << CalculateDeviceScale();;
  NzLog("Set NzChromium window to be required size: %s", ss.str().c_str());

  display::ManagedDisplayInfo di = display::ManagedDisplayInfo::CreateFromSpec(ss.str());
  std::vector<display::ManagedDisplayInfo> displays;
  displays.push_back(di);
  base::PostTaskWithTraits(FROM_HERE, { content::BrowserThread::IO }, base::Bind(&NzosPlatformThread::EventResized, displays));
#else
  gfx::Size sz(WindowWidth, WindowHeight);
  content::Shell::platform()->ResizeWindow(sz);
#endif

  if (NzConfigGetBool("NzChromium.RefreshOnResize", true)) {
    LOG(ERROR) << "Refreshing on resize";
    NzosEventFactory::GetInstance()->EventConverter()->KeyNotify(QZ_KEY_OP_DOWN, QZ_KEY_FLAGS_CTRL_LEFT | QZ_KEY_FLAGS_SHIFT_LEFT, 0x0072);  // KeyDown r
    NzosEventFactory::GetInstance()->EventConverter()->KeyNotify(QZ_KEY_OP_UP, QZ_KEY_FLAGS_CTRL_LEFT | QZ_KEY_FLAGS_SHIFT_LEFT, 0x0072);  // KeyUp   r
  }
}

void NzosPlatformThread::EventShutdown(uint32_t u32Reason) {
  NzLog("EventShutdown... received on instance thread");

  if (NzosEventFactory::GetInstance() && NzosEventFactory::GetInstance()->EventConverter()) {
    NzLog("EventShutdown... inject key sequence to shutdown chrome");
    NzosEventFactory::GetInstance()->EventConverter()->KeyNotify(QZ_KEY_OP_DOWN, QZ_KEY_FLAGS_CTRL_LEFT | QZ_KEY_FLAGS_SHIFT_LEFT, 0x0077);  // KeyDown w
    NzosEventFactory::GetInstance()->EventConverter()->KeyNotify(QZ_KEY_OP_UP, QZ_KEY_FLAGS_CTRL_LEFT | QZ_KEY_FLAGS_SHIFT_LEFT, 0x0077);  // KeyUp   w
  } else {
    base::PostTaskWithTraits(FROM_HERE, { content::BrowserThread::IO }, base::Bind(&NzosPlatformThread::NzosEventShutdown, u32Reason));
    // Instance()->message_loop()->RunTask(new base::PendingTask(FROM_HERE, base::Bind(&NzosPlatformThread::NzosEventShutdown, u32Reason)));
    NzLog("Posted NzosEventShutdown event to message loop");
  }
}

void NzosPlatformThread::sEventClipboardCopyComplete(uint32_t u32Format, uint32_t u32Error) {
}

static bool pasteInProgress = false;
void NzosPlatformThread::EventClipboardNotify(uint32_t u32Formats,
    uint32_t u32Size)
{
  NzLog("EventClipboardNotify: %d, %d", u32Formats, u32Size);

  if (u32Size > 0)
  {
    if (!pasteInProgress)
    {
      pasteInProgress = true;

      // No delayed rendering
      NzClipboardPaste(u32Formats);
    }
    else
    {
      NzLog("Paste in Progress?");
    }
  }
}

void NzosPlatformThread::HandleClipboardPaste(uint32_t u32Format, const std::string& data)
{
  //TODOSJ
  /*
  NzLog("HandleClipboardPaste");
  Clipboard* clipboard = Clipboard::GetForCurrentThread();
  if (clipboard && (u32Format != 0))
  {
    switch (u32Format) {
      case QZ_CLIPBOARD_FORMAT_TEXT: {
        std::string clipboard_string;
        ClipboardType type = CLIPBOARD_TYPE_COPY_PASTE;

        NzLog("Paste: %s", data.c_str());
        clipboard->WriteTextNz(data.c_str(), data.size());
      }
      break;
      case QZ_CLIPBOARD_FORMAT_IMAGE: {
        std::string clipboard_string;
        ClipboardType type = CLIPBOARD_TYPE_COPY_PASTE;

        const uint8_t* buff = (const uint8_t*)data.c_str();
        const tBMPFileHeader* fileHdr = (tBMPFileHeader*)buff;

        const tBMPInfoHeader* infoHdr = (tBMPInfoHeader*)(buff + fileHdr->offsetBits);

        SkImageInfo skInfo = SkImageInfo::Make(infoHdr->width, infoHdr->height, kRGBA_8888_SkColorType,
            kPremul_SkAlphaType);

        // set image pointer after file header, info header and RGB quads
        const uint8_t* ptr = buff + fileHdr->offsetBits + infoHdr->size + sizeof(tRGBQuads);
        uint32_t width = 4 * infoHdr->width;
        DumpHex(ptr, width);
        NzLogVerbose("");
        DumpHex(ptr + width, width);
        NzLogVerbose("");
        DumpHex(ptr + width*2, width);
        SkBitmap b;
        b.setInfo(skInfo, width);
        b.allocPixels();
        // Flip the image
        const uint8_t* srcptr = ptr + ((infoHdr->height - 1) * width);
        for (int y = 0; y < infoHdr->height; y++)
        {
          uint8_t* pixelptr = (uint8_t*)b.getAddr(0, y);
          memcpy((void*)pixelptr, (void*)srcptr, width);
          srcptr -= width;
        }

        clipboard->WriteBitmapNz(b);

      }
      break;
    }
  }
  */
}

void NzosPlatformThread::EventClipboardPaste(uint32_t u32Format, const std::string& data)
{
  NzLog("EventClipboardPaste");

  base::PostTaskWithTraits(FROM_HERE, { content::BrowserThread::IO }, base::Bind(&NzosPlatformThread::HandleClipboardPaste,
      weak_factory_.GetWeakPtr(), u32Format, data));
}

void NzosPlatformThread::sEventClipboardPaste(uint32_t u32Format, uint32_t u32Size, const uint8_t* pData)
{
  NzLog("sEventClipboardPaste");
  // std::string data((const char*)pData, (size_t)u32Size);

  // // Not sure how to get this directly on the UI thread, so I am proxying through NzosPlatformThread::message_loop
  // Instance()->message_loop()->RunTask(new base::PendingTask(FROM_HERE, base::Bind(&NzosPlatformThread::EventClipboardPaste,
  //     NzosPlatformThread::Instance()->weak_factory_.GetWeakPtr(), u32Format, data)));
  // pasteInProgress = false;
}

void NzosPlatformThread::sEventMicrophone(uint32_t event, uint32_t microphoneId, const char* error,
    uint32_t u32DataSize, const uint8_t* pData)
{
//  NzLog("sEventMicrophone");
// TODOSJ
  // media::AudioManagerNzOS::EventMicrophone(event, microphoneId, error, u32DataSize, pData);
}

void NzosPlatformThread::sEventCamera(uint32_t event, uint32_t cameraId, const char* error,
    uint32_t u32DataSize, const uint8_t* pData)
{
  // TODOSJ
  // media::VideoCaptureDeviceNzos* device =
  //     NzosPlatformThread::Instance()->GetVideoCaptureDevice(cameraId);

  // if (device)
  // {
  //   device->EventCamera(event, cameraId, error, u32DataSize, pData);
  // }
  // else
  // {
  //   NzLog("No camera device found");
  // }
}

void NzosPlatformThread::sEventLocation(uint32_t u32LocatorId, uint32_t u32Flags,
    double Latitude, double Longitude, double Altitude,
    double Accuracy, double Bearing, double Speed)
{
  NzLog("sEventLocation: %f/%f, %x", Latitude, Longitude, u32Flags);
  if (Instance()->GetLocationProvider())
  {
    device::mojom::Geoposition position;
    position.latitude = Latitude;
    position.longitude = Longitude;
    if (u32Flags & QZ_LOCATION_HAS_ACCURACY)
      position.accuracy = Accuracy;
    if (u32Flags & QZ_LOCATION_HAS_ALTITUDE)
    {
      position.altitude = Altitude;
      if (u32Flags & QZ_LOCATION_HAS_ACCURACY)
        position.altitude_accuracy = Accuracy;
    }
    if (u32Flags & QZ_LOCATION_HAS_BEARING)
      position.heading = Bearing;
    if (u32Flags & QZ_LOCATION_HAS_SPEED)
      position.speed = Speed;
    position.error_code = device::mojom::Geoposition_ErrorCode::NONE;

    position.timestamp = base::Time::Now();

    Instance()->GetLocationProvider()->NotifyCallback(position);
  }
}

const char* NzosPlatformThread::GetPrintableState(NzosPlatformState state) {
  switch (state) {
    case Startup:            return "Startup";
    case Initialized:        return "Initialized";
    case DeviceConnected:    return "DeviceConnected";
    case AppConnected:       return "AppConnected";
    case AppVisible:         return "AppVisible";
    case DeviceDisconnected: return "DeviceDisconnected";
    case Shutdown:           return "Shutdown";
  }
  return "INVALID_STATE";
}

void NzosPlatformThread::NzosInit() {
  LOG(ERROR) << "NzosInit: ";

  // Set event callbacks
  NzEventCallbacks callbacks;
  memset(&callbacks, 0, sizeof(callbacks));
  callbacks.NzEventConnected                = NzosPlatformThread::EventConnected;
  callbacks.NzEventDisconnected             = NzosPlatformThread::EventDisconnected;
  callbacks.NzEventKeyboard                 = NzosPlatformThread::EventKeyboard;
  callbacks.NzEventMouse                    = NzosPlatformThread::EventMouse;
  callbacks.NzEventShutdown                 = NzosPlatformThread::EventShutdown;
  callbacks.NzEventVisible                  = NzosPlatformThread::EventVisible;
  callbacks.NzEventResize                   = NzosPlatformThread::EventResize;
  callbacks.NzEventJoystick                 = NzosPlatformThread::EventJoystick;
  callbacks.NzEventDRMKeyRequest            = NzosPlatformThread::EventDRMKeyRequest;
  callbacks.NzEventDevicePropertiesEx       = NzosPlatformThread::EventDevicePropertiesEx;
  callbacks.NzEventTouchEx                  = NzosPlatformThread::EventTouch;
  callbacks.NzEventClipboardCopyComplete    = NzosPlatformThread::sEventClipboardCopyComplete;
  callbacks.NzEventClipboardNotify          = NzosPlatformThread::EventClipboardNotify;
  callbacks.NzEventClipboardPaste           = NzosPlatformThread::sEventClipboardPaste;
  callbacks.NzEventMicrophone               = NzosPlatformThread::sEventMicrophone;
  callbacks.NzEventCamera                   = NzosPlatformThread::sEventCamera;
  callbacks.NzEventLocation                 = NzosPlatformThread::sEventLocation;
  NzSetCallbacks(callbacks, sizeof(callbacks));

  // Get the executable name as we use it as the default config file name
  std::string argv0 = base::CommandLine::ForCurrentProcess()->GetProgram().value();
  size_t n = argv0.find_last_of("/\\");
  if (n != std::string::npos)
    argv0 = argv0.substr(n+1);

  // Create nzargc and nzargv
  LOG(ERROR) << "Create args for NzInit";
  static const char* argv[32];
  std::vector<std::string> args = base::CommandLine::ForCurrentProcess()->original_argv();
  int argc = args.size();
  for(int i = 0; i < argc; i++) {
    argv[i] = args[i].c_str();
    LOG(ERROR) << argv[i];
  }

  NzInit(argc, argv, NULL, NULL);
  LOG(ERROR) << "NzInit complete";

  // Don't capture log only if the switch is present with value "no"
  const base::CommandLine* cmd_line = base::CommandLine::ForCurrentProcess();
  if (!cmd_line->HasSwitch(switches::kNzCaptureLog)) {
    logging::SetLogMessageHandler(NzLogMessageHandlerFunction);
  } else {
    std::string capture = cmd_line->GetSwitchValueASCII(switches::kNzCaptureLog);
    if (strcmp(capture.c_str(), "no") != 0) {
      logging::SetLogMessageHandler(NzLogMessageHandlerFunction);
    }
  }

  NzosPlatformThread::Instance()->SetState(NzosPlatformState::Initialized);
}

void NzosPlatformThread::NzosEventShutdown(uint32_t u32Reason) {
  NzLog("Shutting down NzOS (reason:%d), notify chrome/shell", u32Reason);
  NzosPlatformThread::Instance()->SetState(NzosPlatformState::Shutdown);
  NzLog("Proceed with chrome/shell shutdown");

#if defined(OS_CHROMEOS)
  // Now shutdown chrome
  //TODOSJ
  // chrome::CloseAllBrowsersAndQuit();
#else
  // Now shutdown content_shell
  content::ImmediateShutdownAndExitProcess();
#endif
}

void NzosPlatformThread::NzosSetInitialWindowSize(uint32_t handle) {
  NzLog("Set initial window size, %dx%d", g_Width, g_Height);
  if (NzosPlatformThread::Instance()->IsAppConnected())
    NzosEventFactory::GetInstance()->EventConverter()->WindowResized(handle, g_Width, g_Height); // TODO get the widget handle and pass it in here
}

void NzosPlatformThread::NzosClipboardNotify(uint32_t u32Format)
{
  if (NzosPlatformThread::Instance()->IsAppConnected()) {
    uint32_t nzFormat = ChromeToNzFormats(u32Format);
    NzLogVerbose("NzosClipboardNotify: chrome formats: %d, nzos formats: %d", u32Format, nzFormat);
    if (nzFormat != 0)
    {
      Clipboard* clipboard = Clipboard::GetForCurrentThread();
      if (clipboard && (u32Format != 0))
      {
        switch (nzFormat) {
          case QZ_CLIPBOARD_FORMAT_TEXT: {
            std::string clipboard_string;
            ClipboardType type = CLIPBOARD_TYPE_COPY_PASTE;
            if (clipboard->IsFormatAvailable(ui::Clipboard::GetPlainTextWFormatType(),
                type)) {
              clipboard->ReadAsciiText(type, &clipboard_string);
    
              NzLogVerbose("Clipped: %s", clipboard_string.c_str());
    
              // malloc buffer that is freed by platform
              uint8_t* buff = (uint8_t*)malloc(clipboard_string.size() + 1);
              memset(buff, 0, clipboard_string.size() + 1);
              memcpy(buff, clipboard_string.c_str(), clipboard_string.size());
    
              NzClipboardCopy(QZ_CLIPBOARD_FORMAT_TEXT,
                  (uint32_t)clipboard_string.size(), buff);
            }
          }
          break;
          case QZ_CLIPBOARD_FORMAT_IMAGE: {
            ClipboardType type = CLIPBOARD_TYPE_COPY_PASTE;
            if (clipboard->IsFormatAvailable(ui::Clipboard::GetBitmapFormatType(),
                type)) {
    
              SkBitmap b = clipboard->ReadImage(type);
              const SkImageInfo& info = b.info();
              NzLog("Image type: %d, %d", info.colorType(), info.alphaType());
              if (((info.colorType() == kRGBA_8888_SkColorType) ||
                  (info.colorType() == kBGRA_8888_SkColorType)) &&
                  (info.alphaType() == kPremul_SkAlphaType))
              {
                int width = info.width() * 4;
                int imageSize = width * info.height();
                int allocSize = sizeof(tBMPFileHeader) + sizeof(tBMPInfoHeader) + sizeof(tRGBQuads) +
                    imageSize;
    
                // malloc buffer that is freed by platform
                uint8_t* imageBuffer = (uint8_t*)malloc(imageSize);
                tBMPFileHeader* fileHdr = (tBMPFileHeader*)imageBuffer;
                fileHdr->type = 0x4D42; // "BM"
                fileHdr->size = allocSize;
                fileHdr->reserved1 = 0;
                fileHdr->reserved2 = 0;
                fileHdr->offsetBits = sizeof(tBMPFileHeader);
    
                tBMPInfoHeader* infoHdr = (tBMPInfoHeader*)(fileHdr + 1);
                memset((void*)infoHdr, 0, sizeof(tBMPInfoHeader));
                infoHdr->size = sizeof(tBMPInfoHeader);
                infoHdr->width = info.width();
                infoHdr->height = info.height();
                infoHdr->planes = 1;
                infoHdr->bitCount = 32;
                infoHdr->compression = 3;
                infoHdr->sizeImage = imageSize;
    
                tRGBQuads* quads = (tRGBQuads*)(infoHdr + 1);
                quads->red = 0x00ff0000;
                quads->green = 0x0000ff00;
                quads->blue = 0x000000ff;
    
                // Flip the image
                const uint8_t* srcptr = (uint8_t*)(quads + 1) + ((infoHdr->height - 1) * width);
                for (uint32_t y = 0; y < infoHdr->height; y++)
                {
                  uint8_t* pixelptr = (uint8_t*)b.getAddr(0, y);
                  memcpy((void*)srcptr, (void*)pixelptr, width);
                  srcptr -= width;
                }
    
                NzLogVerbose("tBMPFileHeader");
                DumpHex((uint8_t*)fileHdr, sizeof(tBMPFileHeader));
                NzLogVerbose("tBMPInfoHeader");
                DumpHex((uint8_t*)infoHdr, sizeof(tBMPInfoHeader));
                NzLogVerbose("tRGBQuads");
                DumpHex((uint8_t*)quads, sizeof(tRGBQuads));
                NzLogVerbose("Row 1");
                DumpHex(srcptr, width);
                NzLogVerbose("Row 2");
                DumpHex(srcptr + width, width);
                NzLogVerbose("Row 3");
                DumpHex(srcptr + width * 2, width);
    
                NzClipboardCopy(QZ_CLIPBOARD_FORMAT_IMAGE,
                    (uint32_t)allocSize, imageBuffer);
              }
              else
              {
                NzLog("Image type not supported");
              }
            }
          }
          break;
        }
      }
    }
  }
}

bool NzosPlatformThread::VirtualKeyboardEnabled()
{
  return NzConfigGetBool("NzChromium.ShowVirtualKeyboard", true);
}

void NzosPlatformThread::ShowVirtualKeyboard()
{
  static char mobileKeyboard[32] = {0};
  static char desktopKeyboard[32] = {0};
  if (strlen(mobileKeyboard) == 0) {
    NzConfigGet("NzChromium.MobileVirtualKeyboard",
        mobileKeyboard, sizeof(mobileKeyboard) - 1, "PhoneKbd_lower");
  }
  if (strlen(desktopKeyboard) == 0) {
    NzConfigGet("NzChromium.DesktopVirtualKeyboard",
        desktopKeyboard, sizeof(desktopKeyboard) - 1, "QWERTY");
  }

  if (NzConfigGetBool("NzChromium.ShowVirtualKeyboard", true)) {
	  NzKeyboardShow("");
  }
}

void NzosPlatformThread::HideVirtualKeyboard()
{
  if (NzConfigGetBool("NzChromium.ShowVirtualKeyboard", true)) {
    NzKeyboardHide();
  }
}


void NzosPlatformThread::SetLocationProvider(content::NzosLocationProvider* provider,
    bool high_accuracy)
{
  // This is not owned by this class
  locationProvider_ = provider;

  if (provider)
  {
    NzLog("SetLocationProvider: %s accuracy", (high_accuracy ? "High " : "Normal "));
    char strValue[128];
    strValue[0] = '\0';
    uint32_t u32LocatorId = 0;
    NzGetDeviceProperty(e_QzPropertyCategory_Locator, u32LocatorId,
            e_QzPropertyLocator_Name, strValue, sizeof(strValue));

    if (strValue[0] != '\0')
    {
        uint64_t minTime = NzConfigGetInt("NzChromium.LocationMinTime", 1);
        uint32_t minDist = NzConfigGetInt("NzChromium.LocationMinDist", 5);

        NzLocationGetLastKnown(u32LocatorId);
        NzLocationRequestUpdates(u32LocatorId,
                minTime*1000*1000,
                minDist,
                (high_accuracy ? QZ_LOCATION_HIGH_ACCURACY : QZ_LOCATION_BALANCED_POWER_ACCURACY));
    }
  }
}

void NzosPlatformThread::RequestLocationRefresh()
{
  NzLog("RequestLocationRefresh");

  char strValue[128];
  strValue[0] = '\0';
  uint32_t u32LocatorId = 0;
  NzGetDeviceProperty(e_QzPropertyCategory_Locator, u32LocatorId,
          e_QzPropertyLocator_Name, strValue, sizeof(strValue));

  if (strValue[0] != '\0')
  {
    NzLocationGetLastKnown(u32LocatorId);
  }
}

void NzosPlatformThread::SetFocusToApp() {
  if (app_has_focus_)
    return;

  NzLog("Set focus for app");
  EventMouse(QZ_MOUSE_OP_DOWN, QZ_MOUSE_FLAGS_LEFT, 1, 1);
  EventMouse(QZ_MOUSE_OP_UP,   QZ_MOUSE_FLAGS_LEFT, 1, 1);
}

void NzosPlatformThread::SetState(NzosPlatformState state) {
  NzLog("STATE CHANGED %s to %s", GetPrintableState(state_), GetPrintableState(state));
  state_ = state;
}

bool NzosPlatformThread::GetScrollVector(int32_t& mvX, int32_t& mvY) {
  bool validMV = mvX_ != 0 || mvY_ != 0;
  if (validMV) {
    mvX  = mvX_;
    mvY  = mvY_;
    mvX_ = 0;
    mvY_ = 0;
  }
 
  return validMV;
}

void NzosPlatformThread::SetScrollVector(int32_t mvX, int32_t mvY) {
  // NzLog("Current MV (%d, %d)  New MV (%d, %d)  Updated MV (%d, %d)", mvX_, mvY_, mvX, mvY, mvX_ + mvX, mvY_ + mvY);
  mvX_ += mvX;
  mvY_ += mvY;
}

bool NzosPlatformThread::DisableFullScreenNotification() {
  return NzConfigGetBool("NzChromium.DisableFullScreenNotification", true);
}

//TODOSJ
// void NzosPlatformThread::SetVideoCaptureDevice(uint32_t id,
//     media::VideoCaptureDeviceNzos* device)
// {
//   video_capture_devices_[id] = device;
// }

// media::VideoCaptureDeviceNzos* NzosPlatformThread::GetVideoCaptureDevice(uint32_t id)
// {
//   std::map<uint32_t, media::VideoCaptureDeviceNzos*>::iterator it =
//       video_capture_devices_.find(id);

//   media::VideoCaptureDeviceNzos* device = nullptr;
//   if (it != video_capture_devices_.end())
//   {
//     device = (*it).second;
//   }

//   return device;
// }

// void NzosPlatformThread::RemoveVideoCaptureDevice(uint32_t id)
// {
//   video_capture_devices_.erase(id);
// }

float NzosPlatformThread::CalculateDeviceScale() {

  float devScale = 1.0;

  char configname[100];
  sprintf(configname, "NzApp.DipScale[%s]", sDeviceType.c_str());
  int dipScale = NzConfigGetInt(configname, ~0);

  if (dipScale != ~0) {
    NzLog("Scaling for %s : %d / %d", sDeviceType.c_str(), g_DevDpi, dipScale);
    // Calc devScale (covert dpi to float first)
    devScale = ((g_DevDpi * 1.0) / dipScale);
  } else {
    int scale = NzConfigGetInt("NzApp.DeviceScale", 0);
    NzLog("Scaling for %s : %d", sDeviceType.c_str(), scale);
    if (scale != 0) {
      devScale = (scale * 1.0) / 100;
    }
  }

  return devScale;
}

std::string NzosPlatformThread::GetSwVersion() {
  std::string version = "NzOS ";
  version += NzGetVersion();
  return version;
}

}  // namespace ui
