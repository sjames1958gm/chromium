/*
*  nzos_surface_skia.cc
*
*  Copyright (c) 2012 Netzyn Inc. All rights reserved.
*  Use of this source code maybe governed by a BSD-style license in the future.
*  Portions of this source code maybe covered by certain patents.
*
*  NzOS Application Platform Environment
*
*/

#include "base/debug/stack_trace.h"
#include "ui/gfx/native_widget_types.h"

#include "nzos_surface_skia.h"
#include "nzos_platform_thread.h"

#include "third_party/nzos/include//NzApe.h"
    
namespace ui {

NzosSurfaceSkia::NzosSurfaceSkia(uint32_t window_id) : window_id_(window_id), fb_(NULL), fb_width_(0), fb_height_(0), fb_stride_(0) {

  NzLog("APP SURFACE created %d", window_id_);
}

NzosSurfaceSkia::~NzosSurfaceSkia() {
  NzLog("APP SURFACE deleted");
}

void NzosSurfaceSkia::ResizeCanvas(const gfx::Size& viewport_size) {
  NzLog("NzosSurfaceSkia::ResizeCanvas");
  uint32_t* fb;
  uint32_t encoder_width, encoder_height, flags;
  NzGetDisplayInfoEx(fb, fb_width_, fb_height_, fb_stride_, encoder_width, encoder_height, flags);

  NzLog("ResizeCanvas: 0x%x, 0x%x, %dx%d - %d, encoder: %dx%d, viewport: %dx%d", fb, fb_, fb_width_, fb_height_, fb_stride_, encoder_width, encoder_height, viewport_size.width(), viewport_size.height());

  fb_ = fb;

  SkImageInfo info = SkImageInfo::Make(viewport_size.width(),
                                       viewport_size.height(),
                                       kBGRA_8888_SkColorType,
                                       kPremul_SkAlphaType);

  surface_ = SkSurface::MakeRasterDirect(info, fb_, fb_stride_ * 4);
  
  NzosPlatformThread::Instance()->SetFocusToApp();
}  
  
sk_sp<SkSurface> NzosSurfaceSkia::GetSurface() {
  // NzLog("NzosSurfaceSkia::GetSurface");
  if (NzosPlatformThread::Instance()->IsAppConnected())
      NzBeginDraw();
  return sk_sp<SkSurface>(surface_);
}

void NzosSurfaceSkia::PresentCanvas(const gfx::Rect& damage) {
  // NzLog("NzosSurfaceSkia::PresentCanvas");
  if (NzosPlatformThread::Instance()->IsAppConnected()) {
    int32_t X, Y;
    if (NzosPlatformThread::Instance()->GetScrollVector(X, Y))
      NzEndDrawWithScrollVector(fb_, X, Y);
    else
      NzEndDraw(fb_);
  }
}
  
std::unique_ptr<gfx::VSyncProvider> NzosSurfaceSkia::CreateVSyncProvider() {
  NzLog("CreateVSyncProvider()");
  return nullptr;
}

} // namespace ui
