/*
*  nzos_display_browser.h
*
*  Copyright (c) 2012 Netzyn Inc. All rights reserved.
*  Use of this source code maybe governed by a BSD-style license in the future.
*  Portions of this source code maybe covered by certain patents.
*
*  NzOS Application Platform Environment
*
*/

#ifndef __NZOS_DISPLAY_BROWSER_H__
#define __NZOS_DISPLAY_BROWSER_H__

#include <map>
#include "ui/ozone/public/surface_factory_ozone.h"
#include "ui/ozone/public/surface_ozone_canvas.h"
#include "nzos_window_change_observer.h"

namespace ui {

class NzosPlatformWindow;

class NzosDisplayBrowser : public ui::SurfaceFactoryOzone, ui::NzosWindowChangeObserver {
 public:
  NzosDisplayBrowser();
  ~NzosDisplayBrowser() override;
  std::unique_ptr<SurfaceOzoneCanvas> CreateCanvasForWidget(gfx::AcceleratedWidget w) override;
  // TODOSJ
  // bool LoadEGLGLES2Bindings(AddGLLibraryCallback add_gl_library, SetGLGetProcAddressProcCallback set_gl_get_proc_address) override;

  void OnWindowClose(uint32_t windowhandle)  override;
  void OnWindowResized(uint32_t windowhandle, uint32_t width, uint32_t height) override;

  void AddWindow(NzosPlatformWindow* window, uint32_t id);
  void DeleteWindow(NzosPlatformWindow* window, uint32_t id);
 
  static NzosDisplayBrowser* GetInstance() { return instance_; }

 private:
  typedef std::map<uint32_t, NzosPlatformWindow*> PlatformWindowMap;
  PlatformWindowMap          windows_;

  static NzosDisplayBrowser* instance_;

  DISALLOW_COPY_AND_ASSIGN(NzosDisplayBrowser);
};

}  // namespace ui

#endif  // __NZOS_DISPLAY_BROWSER_H__
