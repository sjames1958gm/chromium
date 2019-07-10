/*
*  nzos_platform_thread.h
*
*  Copyright (c) 2012 Netzyn Inc. All rights reserved.
*  Use of this source code maybe governed by a BSD-style license in the future.
*  Portions of this source code maybe covered by certain patents.
*
*  NzOS Application Platform Environment
*
*/

#include "base/threading/thread.h"
#include "ui/events/keycodes/keyboard_codes.h"
#include "ui/ozone/platform/nzos/nzos_platform_interface.h"

#include <map>

namespace display {
class ManagedDisplayInfo;
}

namespace media {
//TODOSJ
// class VideoCaptureDeviceNzos;
}

namespace content {
class NzosLocationProvider;
}

namespace ui {

enum NzosPlatformState { 
  Startup, 
  Initialized,
  DeviceConnected,
  AppConnected,
  AppVisible,
  DeviceDisconnected,
  Shutdown
};

class NzosPlatformThread : public base::Thread {
 public:
  void SetInterface(NzosPlatformInterface* intf);
  bool IsDeviceConnected() { return ((state_ == NzosPlatformState::DeviceConnected) || (state_ == NzosPlatformState::AppVisible)); };
  bool IsAppConnected();
  bool IsAppVisible();
  bool IsShutdown()        { return (state_ == NzosPlatformState::Shutdown); }
  void SetState(NzosPlatformState state);
  void SetFocusToApp();
  const char* GetPrintableState(NzosPlatformState state);
  void SetScrollVector(int32_t mvX, int32_t mvY); 
  bool GetScrollVector(int32_t& mvX, int32_t& mvY);
  bool DisableFullScreenNotification();

//TODOSJ
//   void SetVideoCaptureDevice(uint32_t id, media::VideoCaptureDeviceNzos* device);
//   media::VideoCaptureDeviceNzos* GetVideoCaptureDevice(uint32_t id);
//   void RemoveVideoCaptureDevice(uint32_t id);

  static NzosPlatformThread* Instance();

  // NzApe callback functions
  static void EventConnected(const char* pstrURL);
  static void EventDisconnected();
  static void EventDevicePropertiesEx(void* pNzDevice);
  static void EventDRMKeyRequest(uint32_t u32SessionId, uint32_t u32KeyRqstId, const uint8_t* pOpaqueData, uint32_t u32OpaqueDataLen, const char* pUrl, uint32_t u32UrlLen);
  static void EventDRMKeyResponseAck(uint32_t u32SessionId, uint32_t u32Scheme, uint32_t u32KeyRqstId, const char* response, uint32_t u32ResponseLen, const char* keySetId, uint32_t u32KeySetIdLen);
  static void EventKeyboard(uint32_t Op, uint32_t u32Flags, uint32_t Key);
  static void EventJoystick(uint32_t u32JoystickId, uint32_t u32AxisCount, uint32_t u32AxisInput, const int32_t* i32AxisValues, uint32_t u32ButtonCount, uint64_t u64ButtonInput, uint64_t u64ButtonStates);
  static void EventMouse(uint32_t Op, uint32_t u32Flags, uint32_t X, uint32_t Y);
  static void EventResize(uint32_t WindowWidth, uint32_t WindowHeight);
  static void EventResized(std::vector<display::ManagedDisplayInfo>);
  static void EventShutdown(uint32_t u32Reason);
  static void EventTouch(uint64_t u64EventTime, uint32_t u32TouchId, uint32_t u32Flags, uint32_t u32Op, uint32_t u32FingerId, uint32_t X, uint32_t Y, double dPressure, uint32_t u32TapFingers);
  static void EventVisible(bool bVisible);
  static void EventClipboardNotify(uint32_t u32Formats, uint32_t u32Size);

  static void sEventClipboardCopyComplete(uint32_t u32Format, uint32_t u32Error);

  static void sEventClipboardPaste(uint32_t u32Format, uint32_t u32Size, const uint8_t* pData);
  void EventClipboardPaste(uint32_t u32Format,  const std::string& data);
  void HandleClipboardPaste(uint32_t u32Format, const std::string& data);

  static void sEventMicrophone(uint32_t event, uint32_t microphoneId, const char* error,
      uint32_t u32DataSize, const uint8_t* pData);

  static void sEventCamera(uint32_t event, uint32_t cameraId, const char* error,
      uint32_t u32DataSize, const uint8_t* pData);

  static void sEventLocation(uint32_t u32LocatorId, uint32_t u32Flags,
      double Latitude, double Longitude, double Altitude,
      double Accuracy, double Bearing, double Speed);

  // Tasks
  static void NzosInit();
  static void NzosEventShutdown(uint32_t u32Reason);
  static void NzosSetInitialWindowSize(uint32_t handle);
  static void NzosClipboardNotify(uint32_t formats);
  static bool VirtualKeyboardEnabled();
  static void ShowVirtualKeyboard();
  static void HideVirtualKeyboard();

  content::NzosLocationProvider* GetLocationProvider() { return locationProvider_; }
  void SetLocationProvider(content::NzosLocationProvider* provider, bool high_accuracy);
  void RequestLocationRefresh();

  // Event handling
  static void NotifyEventKeyboard(uint32_t Op, uint32_t u32Flags, uint32_t Key);
  static void NotifyEventMouse(uint32_t Op, uint32_t u32Flags, uint32_t X, uint32_t Y);

  // Device info
  static float CalculateDeviceScale();

  static void SetDeviceType(std::string dt) { sDeviceType = dt; }
  static void SetIsMobileDevice(bool is_mobile) { sIsMobileDevice = is_mobile; }
  static bool IsMobileDevice() { return sIsMobileDevice; }
  static std::string GetDeviceSw() { return sDeviceSw; }
  static void SetDeviceSw(std::string sw) { sDeviceSw = sw; }
  static void SetAllowUserAgentChange(bool allow) { sAllowUserAgentChange = allow; }
  static bool AllowUserAgentChange() { return sAllowUserAgentChange; }
  static std::string GetSwVersion();

 private:
  NzosPlatformThread();
  ~NzosPlatformThread() override;

 private:

  static std::string sDeviceType;
  static std::string sDeviceSw;
  static bool sAllowUserAgentChange;
  static bool sIsMobileDevice;

  NzosPlatformState state_;
  bool              app_has_focus_;
  int32_t           mvX_;
  int32_t           mvY_;
  //TODOSJ
//   std::map<uint32_t, media::VideoCaptureDeviceNzos*> video_capture_devices_;
  // could be smart pointers? but do they ever get deleted?
  content::NzosLocationProvider* locationProvider_ = nullptr;
  NzosPlatformInterface* platformInterface_ = nullptr;

  base::WeakPtrFactory<NzosPlatformThread> weak_factory_;

};

}  // namespace ui
