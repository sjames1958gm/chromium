/*
*  nzos_cursor_factory.h
*
*  Copyright (c) 2012 Netzyn Inc. All rights reserved.
*  Use of this source code maybe governed by a BSD-style license in the future.
*  Portions of this source code maybe covered by certain patents.
*
*  NzOS Application Platform Environment
*
*/

#ifndef __NZOS_CURSOR_FACTORY_H__
#define __NZOS_CURSOR_FACTORY_H__

#include "ui/base/cursor/ozone/bitmap_cursor_factory_ozone.h"

namespace ui {

class NzosCursorFactory : public BitmapCursorFactoryOzone {
 public:
  NzosCursorFactory();
  ~NzosCursorFactory() override;

  static int GetPlatformCursorType(CursorType type);

};

}  // namespace ui

#endif  // __NZOS_CURSOR_FACTORY_H__
