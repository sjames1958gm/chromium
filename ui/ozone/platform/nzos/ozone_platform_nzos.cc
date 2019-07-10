/*
*  ozone_platform_nzos.cc
*
*  Copyright (c) 2012 Netzyn Inc. All rights reserved.
*  Use of this source code maybe governed by a BSD-style license in the future.
*  Portions of this source code maybe covered by certain patents.
*
*  NzOS Application Platform Environment
*
*/

#include <unistd.h>
#include "base/bind.h"
#include "base/command_line.h"
#include "base/threading/thread.h"
#include "base/task/post_task.h"
#include "content/public/browser/browser_task_traits.h"
#include "content/public/common/content_switches.h"
#include "ui/ozone/public/input_controller.h"
#include "ui/events/system_input_injector.h"
#include "ui/ozone/public/ozone_platform.h"
#include "ui/ozone/public/ozone_switches.h"
#include "ui/platform_window/platform_window_init_properties.h"
#include "ui/events/platform/platform_event_source.h"
#include "ui/gfx/geometry/rect.h"

#include "third_party/nzos/include/NzApe.h"

#include "ui/ozone/platform/nzos/nzos_native_display_delegate.h"
#include "ui/ozone/platform/nzos/nzos_platform_thread.h"
#include "ui/ozone/platform/nzos/nzos_platform_window.h"
#include "ui/ozone/platform/nzos/nzos_cursor_factory.h"
#include "ui/ozone/platform/nzos/nzos_event_factory.h"
#include "ui/ozone/platform/nzos/ozone_platform_nzos.h"

// Non-accelerated surfaces support (Browser process)
#include "ui/ozone/platform/nzos/nzos_display_browser.h"

// Hardware accelerated surfaces support (GPU process)
// #include "ui/ozone/platform/nzos/nzos_display_gpu.h"
#include "ui/ozone/platform/nzos/nzos_channel_browser.h"
// #include "ui/ozone/platform/nzos/nzos_channel_gpu.h"
// #include "ui/ozone/platform/nzos/nzos_x11_interface.h"

namespace ui {
class NzosEventSource : public PlatformEventSource {
public:
  NzosEventSource();
 ~NzosEventSource() override;

private:

 DISALLOW_COPY_AND_ASSIGN(NzosEventSource);
};

NzosEventSource::NzosEventSource() {
}

NzosEventSource::~NzosEventSource() {
}

namespace {

class OzonePlatformNzos : public OzonePlatform {
 public:
  OzonePlatformNzos() {
    is_gpu_process_ = base::CommandLine::ForCurrentProcess()->GetSwitchValueASCII(switches::kProcessType) == switches::kGpuProcess;
    x11_interface_initialized_ = false;
    LOG(ERROR) << "OzonePlatformNzos() : " << (is_gpu_process_ ? "GPU-process" : "BROWSER-process") << " with PID=" << getpid();
  }
  ~OzonePlatformNzos() override {
    // if (x11_interface_initialized_)
    //   NzosX11Interface::GetInstance()->Terminate();
  }

  // OzonePlatform:
  ui::SurfaceFactoryOzone* GetSurfaceFactoryOzone() override {
    ui::SurfaceFactoryOzone* pFactory = NULL;
    if (is_gpu_process_)
      ;//pFactory = gpu_surface_factory_ozone_.get();
    else
      pFactory = surface_factory_ozone_.get();
    return pFactory;
  }
  ui::OverlayManagerOzone* GetOverlayManager() override {
    return nullptr;
  }
  CursorFactoryOzone* GetCursorFactoryOzone() override {
    return cursor_factory_ozone_.get();
  }
  ui::InputController* GetInputController() override {
    return input_controller_.get();
  }
  // GpuPlatformSupport* GetGpuPlatformSupport() override {
  //   TODOSJ
  //   return channel_gpu_.get();
  // }
  GpuPlatformSupportHost* GetGpuPlatformSupportHost() override {
    return channel_browser_.get();
  }
  std::unique_ptr<SystemInputInjector> CreateSystemInputInjector() override {
    return nullptr;  // no input injection support.
  }

  std::unique_ptr<PlatformWindow> CreatePlatformWindow(PlatformWindowDelegate* delegate, PlatformWindowInitProperties properties) override {
    LOG(ERROR) << "CreatePlatformWindow" << " with dimension: " << properties.bounds.width() << "x" << properties.bounds.height();
    NzLog("CreatePlatformWindow()");
    if (delegate) {
      if (channel_browser_)
        channel_browser_->SetWindowDelegate(delegate);
      return std::make_unique<NzosPlatformWindow>(delegate, properties.bounds);
    } else
      return nullptr;
  }

  std::unique_ptr<display::NativeDisplayDelegate> CreateNativeDisplayDelegate() override {
    return std::make_unique<NzosNativeDisplayDelegate>();
  }

  void InitializeUI(const InitParams& args) override {
    LOG(ERROR) << "InitializeForUI()";
    cursor_factory_ozone_ = std::make_unique<NzosCursorFactory>();
    event_factory_ozone_ = std::make_unique<NzosEventFactory>();
    channel_browser_ = std::make_unique<NzosChannelBrowser>();
    surface_factory_ozone_ = std::make_unique<NzosDisplayBrowser>();
    input_controller_ = CreateStubInputController();
    event_source_.reset(new ui::NzosEventSource());
    InitializeNzos();
    NzosPlatformThread::Instance()->SetState(NzosPlatformState::AppConnected);
  }

  void InitializeGPU(const InitParams& params) override {
   LOG(ERROR) << "InitializeGPU()";
  //  channel_gpu_.reset(new NzosChannelGpu());
//    event_factory_ozone_.reset(new NzosEventFactory());
//    gpu_surface_factory_ozone_.reset(new NzosDisplayGpu());
//    x11_interface_initialized_ = NzosX11Interface::GetInstance()->Initialize(NULL);
  }

 private:
  bool InitializeNzos() {
    LOG(ERROR) << "InitializeNzos: " << (is_gpu_process_ ? "GPU-process" : "BROWSER-process") << " with PID=" << getpid();
    NzosPlatformThread::Instance()->Start();
    
    NzosPlatformThread::Instance()->task_runner()->PostTask(FROM_HERE, (base::Bind(&NzosPlatformThread::NzosInit)));

    // Wait for device to connect, before we proceed with drawing stuff on the screen
    while (!NzosPlatformThread::Instance()->IsDeviceConnected() && !NzosPlatformThread::Instance()->IsShutdown())
      sleep(1);

    NzClipboardSubscribe(QZ_CLIPBOARD_FORMAT_TEXT | QZ_CLIPBOARD_FORMAT_IMAGE);

    LOG(ERROR) << "NzOS initialized in " << (is_gpu_process_ ? "GPU process" : "Browser process") << " with PID=" << getpid();
    NzLog("NzOS initialized in %s with PID=%d", is_gpu_process_ ? "GPU process" : "Browser process", (uint32_t)getpid());
    NzLog("Device CONNECTED to app.....!!!");
    return true;
  }

 private:
  bool                                 x11_interface_initialized_;
  bool                                 is_gpu_process_;
  std::unique_ptr<NzosDisplayBrowser>  surface_factory_ozone_;
  // std::unique_ptr<NzosDisplayGpu>      gpu_surface_factory_ozone_;
  std::unique_ptr<NzosEventFactory>    event_factory_ozone_;
  std::unique_ptr<NzosCursorFactory>   cursor_factory_ozone_;
//  std::unique_ptr<NzosChannelGpu>	   channel_gpu_;
  std::unique_ptr<NzosChannelBrowser>  channel_browser_;
  std::unique_ptr<InputController>     input_controller_;
  std::unique_ptr<ui::NzosEventSource> event_source_;

  DISALLOW_COPY_AND_ASSIGN(OzonePlatformNzos);
};

}  // namespace

OzonePlatform* CreateOzonePlatformNzos() {
  return new OzonePlatformNzos();
}

}  // namespace ui
