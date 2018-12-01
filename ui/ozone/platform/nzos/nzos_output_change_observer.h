/*
*  nzos_output_change_observer.h
*
*  Copyright (c) 2012 Netzyn Inc. All rights reserved.
*  Use of this source code maybe governed by a BSD-style license in the future.
*  Portions of this source code maybe covered by certain patents.
*
*  NzOS Application Platform Environment
*
*/

#ifndef __NZOS_OUTPUT_CHANGE_OBSERVER_H__
#define __NZOS_OUTPUT_CHANGE_OBSERVER_H__


namespace ui {

class NzosOutputChangeObserver {
 public:
  virtual void OnOutputSizeChanged(uint32_t width, uint32_t height) = 0;

 protected:
  virtual ~NzosOutputChangeObserver() {}
};

}  // namespace ui

#endif  // __NZOS_OUTPUT_CHANGE_OBSERVER_H__
