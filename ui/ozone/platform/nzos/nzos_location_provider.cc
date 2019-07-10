// Copyright (c) 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ui/ozone/platform/nzos/nzos_location_provider.h"
#include "ui/ozone/platform/nzos/nzos_platform_thread.h"

namespace content {

NzosLocationProvider::NzosLocationProvider() : permission_granted_(false) {
}

void NzosLocationProvider::NotifyCallback(const device::mojom::Geoposition& position) {
  if (!callback_.is_null() && permission_granted_) {
    callback_.Run(this, position);
  }
}

void NzosLocationProvider::SetUpdateCallback(
    const device::LocationProvider::LocationProviderUpdateCallback& callback) {
  callback_ = callback;
}

NzosLocationProvider::~NzosLocationProvider() {
  ui::NzosPlatformThread::Instance()->SetLocationProvider(nullptr, false);
}

void NzosLocationProvider::StartProvider(bool high_accuracy)
{
  ui::NzosPlatformThread::Instance()->SetLocationProvider(this, high_accuracy);
}

void NzosLocationProvider::StopProvider()
{
  ui::NzosPlatformThread::Instance()->SetLocationProvider(nullptr, false);
}

device::mojom::Geoposition& NzosLocationProvider::GetPosition()
{
  return last_position_;
}

void NzosLocationProvider::OnPermissionGranted() {
  permission_granted_ = true;
}

}  // namespace content
