/*
*  nzos_channel_gpu.cc
*
*  Copyright (c) 2012 Netzyn Inc. All rights reserved.
*  Use of this source code maybe governed by a BSD-style license in the future.
*  Portions of this source code maybe covered by certain patents.
*
*  NzOS Application Platform Environment
*
*/

#include "base/logging.h"
#include "ui/ozone/platform/nzos/nzos_channel_gpu.h"

namespace ui {

NzosChannelGpu::NzosChannelGpu() {
  LOG(ERROR) << "NzosChannelGpu::NzosChannelGpu()";
}

NzosChannelGpu::~NzosChannelGpu() {
  LOG(ERROR) << "NzosChannelGpu::~NzosChannelGpu()";
}

void NzosChannelGpu::OnChannelEstablished(IPC::Sender* sender) {
  LOG(ERROR) << "NzosChannelGpu::OnChannelEstablished()";
}

IPC::MessageFilter* NzosChannelGpu::GetMessageFilter() {
  return nullptr;
}

bool NzosChannelGpu::OnMessageReceived(const IPC::Message& message) {
  return false;
}

} // namespace ui

