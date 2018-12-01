/*
*  ozone_platform_nzos.h
*
*  Copyright (c) 2018 Netzyn Inc. All rights reserved.
*  Use of this source code maybe governed by a BSD-style license in the future.
*  Portions of this source code maybe covered by certain patents.
*
*  NzOS Application Platform Environment
*
*/

#ifndef __OZONE_PLATFORM_NZOS_H__
#define __OZONE_PLATFORM_NZOS_H__

namespace ui {

class OzonePlatform;

// Constructor hook for use in ozone_platform_list.cc
OzonePlatform* CreateOzonePlatformNzos();

}  // namespace ui

#endif  // __OZONE_PLATFORM_NZOS_H__
