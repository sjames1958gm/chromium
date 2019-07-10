/*
*  nzos_display_gpu.h
*
*  Copyright (c) 2012 Netzyn Inc. All rights reserved.
*  Use of this source code maybe governed by a BSD-style license in the future.
*  Portions of this source code maybe covered by certain patents.
*
*  NzOS Application Platform Environment
*
*/

#ifndef __NZOS_DISPLAY_GPU_H__
#define __NZOS_DISPLAY_GPU_H__

#if !defined(__STDC_FORMAT_MACROS)
#define __STDC_FORMAT_MACROS
#endif

#include <map>
#include "EGL/egl.h"
#include "ui/gfx/native_widget_types.h"
#include "ui/ozone/public/surface_factory_ozone.h"
#include "ui/ozone/public/surface_ozone_canvas.h"

namespace ui {

class NzosDisplayGpu : public ui::SurfaceFactoryOzone {
 public:
  NzosDisplayGpu();
  ~NzosDisplayGpu() override;

  // SurfaceFactoryOzone:
  // intptr_t GetNativeDisplay() override;
  // scoped_ptr<SurfaceOzoneEGL> CreateEGLSurfaceForWidget(gfx::AcceleratedWidget widget) override;
  // bool LoadEGLGLES2Bindings(AddGLLibraryCallback add_gl_library, SetGLGetProcAddressProcCallback set_gl_get_proc_address) override;
  // const int32* GetEGLSurfaceProperties(const int32* desired_list) override;

 private:
  DISALLOW_COPY_AND_ASSIGN(NzosDisplayGpu);
};

}  // namespace ui

#endif  // __NZOS_DISPLAY_GPU_H__
