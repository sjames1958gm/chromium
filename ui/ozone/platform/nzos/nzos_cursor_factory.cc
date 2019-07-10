/*
*  nzos_cursor_factory.cc
*
*  Copyright (c) 2012 Netzyn Inc. All rights reserved.
*  Use of this source code maybe governed by a BSD-style license in the future.
*  Portions of this source code maybe covered by certain patents.
*
*  NzOS Application Platform Environment
*
*/

#include "ui/ozone/platform/nzos/nzos_cursor_factory.h"

#include "third_party/nzos/include/NzApe.h"
#include "third_party/nzos/include/QzDefines.h"

namespace ui {

NzosCursorFactory::NzosCursorFactory() {
  NzLog("NzosCursorFactory()");
}

NzosCursorFactory::~NzosCursorFactory() {
  NzLog("~NzosCursorFactory()");
}

int NzosCursorFactory::GetPlatformCursorType(CursorType type)
{
  switch (type)
  {
  case CursorType::kPointer:
  case CursorType::kCross:
    return QZ_CURSOR_ARROW;
  case CursorType::kHand:
    return QZ_CURSOR_HAND;
  case CursorType::kIBeam:
    return QZ_CURSOR_BAR;
  case CursorType::kWait:
  case CursorType::kHelp:
  case CursorType::kEastResize:
  case CursorType::kNorthResize:
  case CursorType::kNorthEastResize:
  case CursorType::kNorthWestResize:
  case CursorType::kSouthResize:
  case CursorType::kSouthEastResize:
  case CursorType::kSouthWestResize:
  case CursorType::kWestResize:
    return QZ_CURSOR_ARROW;
  case CursorType::kNorthSouthResize:
    return QZ_CURSOR_RESIZE_NS;
  case CursorType::kEastWestResize:
    return QZ_CURSOR_RESIZE_EW;
  case CursorType::kNorthEastSouthWestResize:
    return QZ_CURSOR_RESIZE_NESW;
  case CursorType::kNorthWestSouthEastResize:
    return QZ_CURSOR_RESIZE_NWSE;
  case CursorType::kColumnResize:
  case CursorType::kRowResize:
  case CursorType::kMiddlePanning:
  case CursorType::kEastPanning:
  case CursorType::kNorthPanning:
  case CursorType::kNorthEastPanning:
  case CursorType::kNorthWestPanning:
  case CursorType::kSouthPanning:
  case CursorType::kSouthEastPanning:
  case CursorType::kSouthWestPanning:
  case CursorType::kWestPanning:
    return QZ_CURSOR_ARROW;
  case CursorType::kMove:
    return QZ_CURSOR_MOVE;
  case CursorType::kVerticalText:
  case CursorType::kCell:
  case CursorType::kContextMenu:
  case CursorType::kAlias:
  case CursorType::kProgress:
  case CursorType::kNoDrop:
  case CursorType::kCopy:
    return QZ_CURSOR_ARROW;
  case CursorType::kNone:
    return QZ_CURSOR_NONE;
  case CursorType::kNotAllowed:
  case CursorType::kZoomIn:
  case CursorType::kZoomOut:
  case CursorType::kGrab:
  case CursorType::kGrabbing:
  case CursorType::kCustom:
  default:
    return QZ_CURSOR_ARROW;
  }
  return QZ_CURSOR_ARROW;
}

}  // namespace ui
