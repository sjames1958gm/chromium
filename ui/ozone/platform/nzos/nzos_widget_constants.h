/*
*  nzos_widget_constants.h
*
*  Copyright (c) 2012 Netzyn Inc. All rights reserved.
*  Use of this source code maybe governed by a BSD-style license in the future.
*  Portions of this source code maybe covered by certain patents.
*
*  NzOS Application Platform Environment
*
*/

#ifndef __NZOS_WIDGET_CONSTANTS_H__
#define __NZOS_WIDGET_CONSTANTS_H__

namespace ui {

enum NzosWidgetState {
    CREATE      = 1,  // Create a new Widget
    SHOW        = 2,  // Widget is visible.
    HIDE        = 3,  // Widget is hidden.
    FULLSCREEN  = 4,  // Widget is in fullscreen mode.
    MAXIMIZED   = 5,  // Widget is maximized,
    MINIMIZED   = 6,  // Widget is minimized.
    RESTORE     = 7,  // Restore Widget.
    ACTIVE      = 8,  // Widget is Activated.
    INACTIVE    = 9,  // Widget is DeActivated.
    RESIZE      = 10, // Widget is Resized.
    DESTROYED   = 11  // Widget is Destroyed.
};

enum NzosWidgetType {
    WINDOW          = 1, // A decorated Window.
    WINDOWFRAMELESS = 2, // An undecorated Window.
    POPUP           = 3  // An undecorated Window, with transient positioning relative to its parent
};

}  // namespace ui

#endif  // __NZOS_WIDGET_CONSTANTS_H__
