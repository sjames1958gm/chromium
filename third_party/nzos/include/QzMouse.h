/*
*  QzMouse.h
*
*  Copyright (c) 2010 Netzyn Inc. All rights reserved.
*  Use of this source code maybe governed by a BSD-style license in the future.
*  Portions of this source code maybe covered by certain patents.
*
*/
#ifndef _QzMouse_h_
#define _QzMouse_h_

#define QZ_MOUSE_OP_MOVE        0
#define QZ_MOUSE_OP_DOWN        1
#define QZ_MOUSE_OP_UP          2
#define QZ_MOUSE_OP_DBLCLICK    3
#define QZ_MOUSE_OP_WHEELUP     4
#define QZ_MOUSE_OP_WHEELDOWN   5
#define QZ_MOUSE_OP_ENTER       6
#define QZ_MOUSE_OP_LEAVE       7
#define QZ_MOUSE_OP_LONG_CLICK  9

#define QZ_MOUSE_FLAGS_NONE    0x0000
#define QZ_MOUSE_FLAGS_LEFT    0x0001
#define QZ_MOUSE_FLAGS_RIGHT   0x0002
#define QZ_MOUSE_FLAGS_MIDDLE  0x0004
#define QZ_MOUSE_FLAGS_BUTTONS (QZ_MOUSE_FLAGS_LEFT|QZ_MOUSE_FLAGS_RIGHT|QZ_MOUSE_FLAGS_MIDDLE)

#endif // _QzMouse_h_
