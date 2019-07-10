/*
*  nzos_x11_interface.h
*
*  Copyright (c) 2012 Netzyn Inc. All rights reserved.
*  Use of this source code maybe governed by a BSD-style license in the future.
*  Portions of this source code maybe covered by certain patents.
*
*  NzOS Application Platform Environment
*
*/

#ifndef __NZOS_X11_INTERFACE_H__
#define __NZOS_X11_INTERFACE_H__

#include <stdbool.h>
#include <stdint.h>
#include <string.h>
#include <X11/Xlib.h>
#include <X11/Xatom.h>
#include <X11/Xutil.h>

namespace ui {

#define NZOS_WINDOW_WIDTH  0x0001
#define NZOS_WINDOW_HEIGHT 0x0002

class NzosX11Interface {
 public:
  static NzosX11Interface* GetInstance();

  bool Initialize(const char* displayName);
  bool Terminate();

  uint32_t CreateWindow(uint32_t width, uint32_t height);
  bool DestroyWindow(uint32_t window_id);
  bool QueryAttributes(uint32_t window_id, int attribute, int* value);
  
  intptr_t GetNativeDisplay();
  intptr_t GetNativeWindow(intptr_t native_window_id);
  bool     ReleaseNativeWindow(intptr_t native_window);

 private:
  NzosX11Interface() { display_ = NULL; }
  virtual ~NzosX11Interface() {}

 private:
  Display* display_;

  static NzosX11Interface* instance_;
};

}  // namespace ui

#endif  // __NZOS_X11_INTERFACE_H__

