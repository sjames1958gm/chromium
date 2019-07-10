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

#ifndef __NZOS_CHANNEL_BROWSER_H__
#define __NZOS_CHANNEL_BROWSER_H__

#include <string>
#include "ui/ozone/public/gpu_platform_support_host.h"

namespace ui {

class NzosEventConverter;
class PlatformWindowDelegate;

class NzosChannelBrowser : public GpuPlatformSupportHost {
 public:
  NzosChannelBrowser();
  ~NzosChannelBrowser() override;

  void SetWindowDelegate(PlatformWindowDelegate* delegate);

  // GpuPlatformSupportHost:

  // Called when the GPU process is spun up.
  // This is called from browser IO thread.
  void OnGpuProcessLaunched(
      int host_id,
      scoped_refptr<base::SingleThreadTaskRunner> ui_runner,
      scoped_refptr<base::SingleThreadTaskRunner> send_runner,
      const base::Callback<void(IPC::Message*)>& sender) override;

  // Called when the GPU process is destroyed.
  // This is called from browser UI thread.
  void OnChannelDestroyed(int host_id) override;

  // Called to handle an IPC message. Note that this can be called from any
  // thread.
  void OnMessageReceived(const IPC::Message& message) override;

  // Called when the GPU service is launched.
  // Called from the browser IO thread.
  void OnGpuServiceLaunched(
      scoped_refptr<base::SingleThreadTaskRunner> host_runner,
      scoped_refptr<base::SingleThreadTaskRunner> io_runner,
      GpuHostBindInterfaceCallback binder,
      GpuHostTerminateCallback terminate_callback) override;

 private:
  NzosEventConverter* event_converter_;
  DISALLOW_COPY_AND_ASSIGN(NzosChannelBrowser);
};

} // namespace ui

#endif // __NZOS_CHANNEL_BROWSER_H__
