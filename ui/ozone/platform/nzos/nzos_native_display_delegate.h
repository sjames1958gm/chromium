// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef __NZOS_NATIVE_DISPLAY_DELEGATE_H__
#define __NZOS_NATIVE_DISPLAY_DELEGATE_H__

#include "base/macros.h"
#include "ui/display/types/native_display_delegate.h"
#include "ui/display/types/gamma_ramp_rgb_entry.h"

namespace ui {

class NzosNativeDisplayDelegate : public display::NativeDisplayDelegate {
 public:
  NzosNativeDisplayDelegate();
  ~NzosNativeDisplayDelegate() override;

  // NativeDisplayDelegate overrides:
  void Initialize() override;
  void TakeDisplayControl(display::DisplayControlCallback callback) override;
  void RelinquishDisplayControl(display::DisplayControlCallback callback) override;
  void GetDisplays(const display::GetDisplaysCallback callback) override;
  void Configure(const display::DisplaySnapshot& output,
                 const display::DisplayMode* mode,
                 const gfx::Point& origin,
                 display::ConfigureCallback callback) override;
  void GetHDCPState(const display::DisplaySnapshot& output,
                    display::GetHDCPStateCallback callback) override;
  void SetHDCPState(const display::DisplaySnapshot& output,
                    display::HDCPState state,
                    display::SetHDCPStateCallback callback) override;
  bool SetColorMatrix(int64_t display_id,
                              const std::vector<float>& color_matrix) override;

  bool SetGammaCorrection(
      int64_t display_id,
      const std::vector<display::GammaRampRGBEntry>& degamma_lut,
      const std::vector<display::GammaRampRGBEntry>& gamma_lut) override;
  void AddObserver(display::NativeDisplayObserver* observer) override;
  void RemoveObserver(display::NativeDisplayObserver* observer) override;
  display::FakeDisplayController* GetFakeDisplayController() override;

 private:
  std::vector<std::unique_ptr<display::DisplaySnapshot>> displays_;

  DISALLOW_COPY_AND_ASSIGN(NzosNativeDisplayDelegate);
};

}  // namespace ui

#endif  // __NZOS_NATIVE_DISPLAY_DELEGATE_H__
