/*
*  nzos_display_browser.cc
*
*  Copyright (c) 2012 Netzyn Inc. All rights reserved.
*  Use of this source code maybe governed by a BSD-style license in the future.
*  Portions of this source code maybe covered by certain patents.
*
*  NzOS Application Platform Environment
*
*/

#include "third_party/nzos/include/NzApe.h"
#include "ui/ozone/platform/nzos/nzos_display_browser.h"
#include "ui/ozone/platform/nzos/nzos_platform_window.h"
#include "ui/ozone/platform/nzos/nzos_surface_skia.h"
#include "ui/platform_window/platform_window_delegate.h"

namespace ui {

NzosDisplayBrowser* NzosDisplayBrowser::instance_ = NULL;

NzosDisplayBrowser::NzosDisplayBrowser() {
  NzLog("NzosDisplayBrowser::NzosDisplayBrowser()");
  instance_ = this;
}

NzosDisplayBrowser::~NzosDisplayBrowser() {
  NzLog("NzosDisplayBrowser::~NzosDisplayBrowser()");
}

void NzosDisplayBrowser::AddWindow(NzosPlatformWindow* window, uint32_t id) {
  NzLog("NzosDisplayBrowser::AddWindow: %d", id);
  windows_[id] = window;
}

void NzosDisplayBrowser::DeleteWindow(NzosPlatformWindow* window, uint32_t id) {
  NzLog("NzosDisplayBrowser::DeleteWindow: %d", id);
  windows_.erase(id);
}

std::unique_ptr<ui::SurfaceOzoneCanvas> NzosDisplayBrowser::CreateCanvasForWidget(gfx::AcceleratedWidget w) {
  NzLog("NzosDisplayBrowser::CreateCanvasForWidget()");
  if (!windows_[w])
    NzLog("Request to create canvas for unidentified widget (%d)", w);
  return std::make_unique<NzosSurfaceSkia>(w);
}

// TODOSJ
// bool NzosDisplayBrowser::LoadEGLGLES2Bindings(AddGLLibraryCallback add_gl_library, SetGLGetProcAddressProcCallback set_gl_get_proc_address) {
//   NzLog("NzosDisplayBrowser::LoadEGLGLES2Bindings()");
//   return false;
// }

void NzosDisplayBrowser::OnWindowClose(uint32_t windowhandle) {
  NzosPlatformWindow* window = windows_[windowhandle];
  if (window) {
    NzLog("Window with handle:%d is being closed", windowhandle);
    window->Delegate()->OnCloseRequest();
  } else {
    NzLog("UNKNOWN Window with handle:%d is being closed", windowhandle);
  }
}

void NzosDisplayBrowser::OnWindowResized(uint32_t windowhandle, uint32_t width, uint32_t height) {
  LOG(ERROR) << "NzosDisplayBrowser::OnWindowResized handle:" << windowhandle << " width:" << width << " height:" << height;
  NzosPlatformWindow* window = windows_[windowhandle];
  if (window) {
    NzLog("Window with handle:%d is being resized -> new size %dx$d", windowhandle, width, height);
    window->SetBounds(gfx::Rect(0, 0, width, height));
  } else {
    NzLog("UNKNOWN Window with handle:%d is being resized -> new size %dx$d", windowhandle, width, height);
  }
}

} // namespace ui
