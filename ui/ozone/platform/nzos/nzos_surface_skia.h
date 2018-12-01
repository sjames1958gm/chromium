/*
*  nzos_surface_skia.h
*
*  Copyright (c) 2012 Netzyn Inc. All rights reserved.
*  Use of this source code maybe governed by a BSD-style license in the future.
*  Portions of this source code maybe covered by certain patents.
*
*  NzOS Application Platform Environment
*
*/

#ifndef __NZOS_SURFACE_SKIA_H__
#define __NZOS_SURFACE_SKIA_H__

#include "ui/gfx/geometry/rect.h"
#include "ui/gfx/geometry/size.h"
#include "ui/gfx/native_widget_types.h"
#include "ui/ozone/public/surface_ozone_canvas.h"
#include "ui/gfx/skia_util.h"
#include "ui/gfx/vsync_provider.h"
#include "third_party/skia/include/core/SkCanvas.h"
#include "third_party/skia/include/core/SkSurface.h"

namespace ui {

class NzosSurfaceSkia : public SurfaceOzoneCanvas {
 public:
  NzosSurfaceSkia(uint32_t window_id); 
  ~NzosSurfaceSkia() override;

  // SurfaceOzoneCanvas overrides:
  void ResizeCanvas(const gfx::Size& viewport_size) override;
  sk_sp<SkSurface> GetSurface() override;
  void PresentCanvas(const gfx::Rect& damage) override;
  std::unique_ptr<gfx::VSyncProvider> CreateVSyncProvider() override;

 private:
  uint32_t                window_id_;
  uint32_t*               fb_;
  uint32_t                fb_width_;
  uint32_t                fb_height_;
  uint32_t                fb_stride_;
  sk_sp<SkSurface> surface_;
};

} // namespace ui

#endif  // __NZOS_SURFACE_SKIA_H__
