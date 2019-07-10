/*
*  nzos_channel_gpu.h
*
*  Copyright (c) 2012 Netzyn Inc. All rights reserved.
*  Use of this source code maybe governed by a BSD-style license in the future.
*  Portions of this source code maybe covered by certain patents.
*
*  NzOS Application Platform Environment
*
*/

#ifndef __NZOS_CHANNEL_GPU_H__
#define __NZOS_CHANNEL_GPU_H__

#include "ui/ozone/public/gpu_platform_support.h"

namespace ui {


class NzosChannelGpu : public GpuPlatformSupport {
 public:
  NzosChannelGpu();
  ~ NzosChannelGpu() override;

  // GpuPlatformSupport:
  void OnChannelEstablished(IPC::Sender* sender) override;
  IPC::MessageFilter* GetMessageFilter() override;
  bool OnMessageReceived(const IPC::Message& message) override;

 private:
  DISALLOW_COPY_AND_ASSIGN(NzosChannelGpu);
};

} // namespace ui

#endif // __NZOS_CHANNEL_GPU_H__
