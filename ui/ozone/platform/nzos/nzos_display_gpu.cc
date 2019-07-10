/*
*  nzos_display.cc
*
*  Copyright (c) 2012 Netzyn Inc. All rights reserved.
*  Use of this source code maybe governed by a BSD-style license in the future.
*  Portions of this source code maybe covered by certain patents.
*
*  NzOS Application Platform Environment
*
*/

#include "base/files/file_path.h"
#include "base/native_library.h"
#include "base/stl_util.h"
#include "nzos_display_gpu.h"
#include "nzos_surface_egl.h"
#include "nzos_surface_skia.h"
#include "nzos_x11_interface.h"
#include "nzos_platform_thread.h"
#include "nzos/NzApe/NzApe.h"

namespace ui {

NzosDisplayGpu::NzosDisplayGpu() {
  LOG(ERROR) << "NzosDisplayGpu::NzosDisplayGpu()" << this;
}

NzosDisplayGpu::~NzosDisplayGpu() {
  LOG(ERROR) << "NzosDisplayGpu::~NzosDisplayGpu()";
}

intptr_t NzosDisplayGpu::GetNativeDisplay() {
  return NzosX11Interface::GetInstance()->GetNativeDisplay();
}

scoped_ptr<ui::SurfaceOzoneEGL> NzosDisplayGpu::CreateEGLSurfaceForWidget(gfx::AcceleratedWidget w) {
  LOG(ERROR) << "NzosDisplayGpu::CreateEGLSurfaceForWidget() id: " << w;
  return make_scoped_ptr<ui::SurfaceOzoneEGL>(new NzosSurfaceEGL(w));
}

bool NzosDisplayGpu::LoadEGLGLES2Bindings(ui::SurfaceFactoryOzone::AddGLLibraryCallback add_gl_library, ui::SurfaceFactoryOzone::SetGLGetProcAddressProcCallback setprocaddress) {
  LOG(ERROR) << "NzosDisplayGpu::LoadEGLGLES2Bindings()";
  base::NativeLibraryLoadError error;

  base::NativeLibrary gles_library = base::LoadNativeLibrary(base::FilePath("libGLESv2.so.2"), &error);
  if (!gles_library) {
    LOG(ERROR) << "Failed to load GLES library: " << error.ToString().c_str();
    return false;
  }

  base::NativeLibrary egl_library = base::LoadNativeLibrary(base::FilePath("libEGL.so.1"), &error);
  if (!egl_library) {
    LOG(ERROR) << "Failed to load EGL library: " << error.ToString().c_str();
    base::UnloadNativeLibrary(gles_library);
    return false;
  }

  GLGetProcAddressProc get_proc_address = reinterpret_cast<GLGetProcAddressProc>(base::GetFunctionPointerFromNativeLibrary(egl_library, "eglGetProcAddress"));
  if (!get_proc_address) {
    LOG(ERROR) << "eglGetProcAddress not found";
    base::UnloadNativeLibrary(egl_library);
    base::UnloadNativeLibrary(gles_library);
    return false;
  }

  setprocaddress.Run(get_proc_address);
  add_gl_library.Run(egl_library);
  add_gl_library.Run(gles_library);
  LOG(ERROR) << "GLESv2 and EGL libraries have been loaded";
  return true;
}

const int32_t* NzosDisplayGpu::GetEGLSurfaceProperties(const int32* desired_list) {
  static const EGLint kConfigAttribs[] = {
    EGL_BUFFER_SIZE, 32,
    EGL_RED_SIZE, 8,
    EGL_GREEN_SIZE, 8,
    EGL_BLUE_SIZE, 8,
    EGL_ALPHA_SIZE, 8,
    EGL_RENDERABLE_TYPE, EGL_OPENGL_ES2_BIT,
    EGL_SURFACE_TYPE, EGL_WINDOW_BIT | EGL_PBUFFER_BIT,
    EGL_NONE
  };

  LOG(ERROR) << "NzosDisplayGpu::GetEGLSurfaceAttributes()";
  return kConfigAttribs;
}

}  // namespace ui
