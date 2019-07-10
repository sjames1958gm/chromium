/*
*  nzos_window_change_observer.h
*
*  Copyright (c) 2012 Netzyn Inc. All rights reserved.
*  Use of this source code maybe governed by a BSD-style license in the future.
*  Portions of this source code maybe covered by certain patents.
*
*  NzOS Application Platform Environment
*
*/

#ifndef __NZOS_WINDOW_CHANGE_OBSERVER_H__
#define __NZOS_WINDOW_CHANGE_OBSERVER_H__

#include <string>

namespace ui {

class NzosWindowChangeObserver {
 public:
  virtual void OnWindowClose(uint32_t windowhandle) = 0;
  virtual void OnWindowResized(uint32_t windowhandle, uint32_t width, uint32_t height) = 0;

 protected:
  virtual ~NzosWindowChangeObserver() {}
};

}  // namespace ui

#endif  // __NZOS_WINDOW_CHANGE_OBSERVER_H__
