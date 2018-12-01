// Copyright (c) 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef NZOS_LOCATION_PROVIDER_BASE_H_
#define NZOS_LOCATION_PROVIDER_BASE_H_

#include "content/common/content_export.h"
#include "services/device/public/cpp/geolocation/location_provider.h"

namespace content {

class CONTENT_EXPORT NzosLocationProvider
    : public device::LocationProvider {
 public:
  NzosLocationProvider();
  ~NzosLocationProvider() override;

  void NotifyCallback(const device::mojom::Geoposition& position);

  void SetUpdateCallback(
    const device::LocationProvider::LocationProviderUpdateCallback& callback) override;
  void StartProvider(bool high_accuracy) override;
  void StopProvider() override;
  device::mojom::Geoposition& GetPosition() override;

  void OnPermissionGranted() override;

 private:
  LocationProviderUpdateCallback callback_;
  device::mojom::Geoposition last_position_;
  bool permission_granted_;

  DISALLOW_COPY_AND_ASSIGN(NzosLocationProvider);
};

}  // namespace content

#endif  // NZOS_LOCATION_PROVIDER_BASE_H_
