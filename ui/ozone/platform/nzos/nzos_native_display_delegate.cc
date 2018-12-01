//

#include "ui/ozone/platform/nzos/nzos_native_display_delegate.h"

#include "base/logging.h"
// #include "ui/ozone/platform/nzos/nzos_display_snapshot_proxy.h"
#include "ui/ozone/platform/drm/common/drm_util.h"
#include "ui/ozone/common/gpu/ozone_gpu_message_params.h"

#include "ui/ozone/platform/nzos/nzos_display_util.h"

namespace ui {

NzosNativeDisplayDelegate::NzosNativeDisplayDelegate() {
}

NzosNativeDisplayDelegate::~NzosNativeDisplayDelegate() {
}

void NzosNativeDisplayDelegate::Initialize() {
  DisplaySnapshot_Params params;
  if (CreateSnapshotFromCommandLine(&params)) {
    DCHECK_NE(display::DISPLAY_CONNECTION_TYPE_NONE, params.type);
    displays_.push_back(CreateDisplaySnapshot(params));
  }
}

void NzosNativeDisplayDelegate::TakeDisplayControl(
    display::DisplayControlCallback callback) {
  NOTREACHED();
}

void NzosNativeDisplayDelegate::RelinquishDisplayControl(
    display::DisplayControlCallback callback) {
  NOTREACHED();
}

void NzosNativeDisplayDelegate::GetDisplays(
    display::GetDisplaysCallback callback) {
  std::vector<display::DisplaySnapshot*> displays;
  for (auto& display : displays_) {
    displays.push_back(display.get());
  }
  std::move(callback).Run(displays);
}

void NzosNativeDisplayDelegate::Configure(
  const display::DisplaySnapshot& output,
  const display::DisplayMode* mode,
  const gfx::Point& origin,
  display::ConfigureCallback callback) {
  NOTREACHED();
}

void NzosNativeDisplayDelegate::GetHDCPState(
  const display::DisplaySnapshot& output,
  display::GetHDCPStateCallback callback) {
    NOTREACHED();
}

void NzosNativeDisplayDelegate::SetHDCPState(
  const display::DisplaySnapshot& output,
  display::HDCPState state,
  display::SetHDCPStateCallback callback) {
  NOTREACHED();
}

bool NzosNativeDisplayDelegate::SetColorMatrix(int64_t display_id, 
  const std::vector<float>& color_matrix) {
  NOTREACHED();
  return false;
}

bool NzosNativeDisplayDelegate::SetGammaCorrection(
    int64_t display_id,
    const std::vector<display::GammaRampRGBEntry>& degamma_lut,
    const std::vector<display::GammaRampRGBEntry>& gamma_lut) {
  NOTREACHED();
  return false;
}

void NzosNativeDisplayDelegate::AddObserver(display::NativeDisplayObserver* observer) {
  NOTREACHED();
}

void NzosNativeDisplayDelegate::RemoveObserver(
    display::NativeDisplayObserver* observer) {
  NOTREACHED();
}

display::FakeDisplayController*
NzosNativeDisplayDelegate::GetFakeDisplayController() {
  return nullptr;
}

}  // namespace ui
