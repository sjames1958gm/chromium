/*
*  nzos_x11_interface.cc
*
*  Copyright (c) 2012 Netzyn Inc. All rights reserved.
*  Use of this source code maybe governed by a BSD-style license in the future.
*  Portions of this source code maybe covered by certain patents.
*
*  NzOS Application Platform Environment
*
*/

#include "ui/ozone/platform/nzos/nzos_x11_interface.h"
#include "third_party/nzos/include/NzApe.h"

namespace ui {

NzosX11Interface* NzosX11Interface::instance_ = NULL;

NzosX11Interface* NzosX11Interface::GetInstance() {
  if (!instance_)
    instance_ = new NzosX11Interface();
  return instance_;
}

bool NzosX11Interface::Initialize(const char* displayName) {
  display_ = XOpenDisplay(displayName);
  if (display_ != NULL)
    NzLog("X interface initialized");
  else
    NzLog("Unable to initialize X interface");
  return display_ != NULL;
}

bool NzosX11Interface::Terminate() {
  XCloseDisplay(display_);
  return true;
}

uint32_t NzosX11Interface::CreateWindow(uint32_t width, uint32_t height) {
  XSetWindowAttributes swa;
  memset(&swa, 0, sizeof(swa));
  swa.event_mask = 0;

  Window window = XCreateWindow(display_,
                                DefaultRootWindow(display_),
                                0,
                                0,
                                width,
                                height,
                                0,
                                CopyFromParent,
                                InputOutput,
                                CopyFromParent,
                                CWEventMask,
                                &swa);

  XMapWindow(display_, window);
  XStoreName(display_, window, "NzosEGL");
  XFlush(display_);
  NzLog("X window created and mapped to display");
  return window;
}

bool NzosX11Interface::DestroyWindow(uint32_t window_id) {
  XDestroyWindow(display_, window_id);
  NzLog("X window destroyed");
  return true;
}

bool NzosX11Interface::QueryAttributes(uint32_t window_id, int attribute, int* value) {
  XWindowAttributes window_attributes;
  switch (attribute) {
    case NZOS_WINDOW_WIDTH:
      XGetWindowAttributes(display_, window_id, &window_attributes);
      *value = window_attributes.width;
      return true;
    case NZOS_WINDOW_HEIGHT:
      XGetWindowAttributes(display_, window_id, &window_attributes);
      *value = window_attributes.height;
      return true;
    default:
      return false;
  }
}

intptr_t NzosX11Interface::GetNativeDisplay(void) {
  NzLog("GetNativeDisplay");
  return reinterpret_cast<intptr_t>(display_);
}

intptr_t NzosX11Interface::GetNativeWindow(intptr_t native_window_id) {
  NzLog("GetNativeWindow");
  return native_window_id;
}

bool NzosX11Interface::ReleaseNativeWindow(intptr_t native_window) {
  NzLog("ReleaseNativeWindow");
  return true;
}

}  // namespace ui
