// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_USB_USB_POLICY_ALLOWED_DEVICES_H_
#define CHROME_BROWSER_USB_USB_POLICY_ALLOWED_DEVICES_H_

#include <map>
#include <memory>
#include <set>
#include <utility>

#include "components/prefs/pref_change_registrar.h"
#include "url/gurl.h"

namespace device {
namespace mojom {
class UsbDeviceInfo;
}  // namespace mojom
}  // namespace device

class PrefService;

// This class is used to initialize a UsbDeviceIdsToUrlsMap from the
// preference value for the WebUsbAllowDevicesForUrls policy. The map
// provides an efficient method of checking if a particular device is allowed to
// be used by the given requesting and embedding origins. Additionally, this
// class also uses |pref_change_registrar_| to observe for changes to the
// preference value so that the map can be updated accordingly.
class UsbPolicyAllowedDevices {
 public:
  // A map of device IDs to a set of GURLs stored in a std::pair. The device IDs
  // correspond to a pair of |vendor_id| and |product_id| integers. The GURLs
  // correspond to a pair of |requesting_url| and |embedding_url| that are
  // allowed to access the device mapped to them.
  using UsbDeviceIdsToUrlsMap =
      std::map<std::pair<int, int>, std::set<std::pair<GURL, GURL>>>;

  // Initializes |pref_change_registrar_| with |pref_service| and adds an
  // an observer for the pref path |kManagedWebUsbAllowDevicesForUrls|.
  explicit UsbPolicyAllowedDevices(PrefService* pref_service);
  ~UsbPolicyAllowedDevices();

  // Checks if |requesting_origin| (when embedded within |embedding_origin|) is
  // allowed to use the device with |device_info|.
  bool IsDeviceAllowed(const GURL& requesting_origin,
                       const GURL& embedding_origin,
                       const device::mojom::UsbDeviceInfo& device_info);

  const UsbDeviceIdsToUrlsMap& map() const { return usb_device_ids_to_urls_; }

 private:
  // Creates or updates the |usb_device_ids_to_urls_| map using the
  // pref at the path |kManagedWebUsbAllowDevicesForUrls|. The existing map is
  // cleared to ensure that previous pref settings are removed.
  void CreateOrUpdateMap();

  // Allow for this class to observe changes to the pref value.
  PrefChangeRegistrar pref_change_registrar_;
  UsbDeviceIdsToUrlsMap usb_device_ids_to_urls_;
};

#endif  // CHROME_BROWSER_USB_USB_POLICY_ALLOWED_DEVICES_H_
