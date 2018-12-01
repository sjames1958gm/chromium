/*
*  nzos_channel_browser.h
*
*  Copyright (c) 2012 Netzyn Inc. All rights reserved.
*  Use of this source code maybe governed by a BSD-style license in the future.
*  Portions of this source code maybe covered by certain patents.
*
*  NzOS Application Platform Environment
*
*/

#include "ui/ozone/platform/nzos/nzos_channel_browser.h"
#include "ui/ozone/platform/nzos/nzos_event_converter.h"
#include "ui/ozone/platform/nzos/nzos_event_factory.h"
#include "third_party/nzos/include/NzApe.h"

namespace ui {

NzosChannelBrowser::NzosChannelBrowser() {
  NzLog("NzosChannelBrowser::NzosChannelBrowser()");
  event_converter_ = new NzosEventConverter();
  NzosEventFactory* event_factory = NzosEventFactory::GetInstance();
  event_factory->SetEventConverter(event_converter_);
  event_converter_->SetWindowChangeObserver(event_factory->GetWindowChangeObserver());
  event_converter_->SetOutputChangeObserver(event_factory->GetOutputChangeObserver());
}

NzosChannelBrowser::~NzosChannelBrowser() {
  NzLog("NzosChannelBrowser::~NzosChannelBrowser()");
  delete event_converter_;
}

void NzosChannelBrowser::SetWindowDelegate(PlatformWindowDelegate* delegate) {
  NzLog("NzosChannelBrowser::SetWindowDelegate()");
  if (event_converter_)
    event_converter_->SetWindowDelegate(delegate);
}

  // Called when the GPU process is spun up.
  // This is called from browser IO thread.
  void NzosChannelBrowser::OnGpuProcessLaunched(
      int host_id,
      scoped_refptr<base::SingleThreadTaskRunner> ui_runner,
      scoped_refptr<base::SingleThreadTaskRunner> send_runner,
      const base::Callback<void(IPC::Message*)>& sender) {
    NzLog("OnGpuServiceLaunched");
      }

  // Called when the GPU process is destroyed.
  // This is called from browser UI thread.
  void NzosChannelBrowser::OnChannelDestroyed(int host_id) {
    NzLog("OnChannelDestroyed");

  }

  // Called to handle an IPC message. Note that this can be called from any
  // thread.
  void NzosChannelBrowser::OnMessageReceived(const IPC::Message& message) {
    NzLog("OnMessageReceived");

  }

  // Called when the GPU service is launched.
  // Called from the browser IO thread.
  void NzosChannelBrowser::OnGpuServiceLaunched(
      scoped_refptr<base::SingleThreadTaskRunner> host_runner,
      scoped_refptr<base::SingleThreadTaskRunner> io_runner,
      GpuPlatformSupportHost::GpuHostBindInterfaceCallback binder,
      GpuPlatformSupportHost::GpuHostTerminateCallback terminate_callback) {
    NzLog("OnGpuServiceLaunched");
      }

} // namespace ui
