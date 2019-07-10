// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef _NZOS_CLIENT_NATIVE_PIXMAP_FACTORY_NZOS_H_
#define _NZOS_CLIENT_NATIVE_PIXMAP_FACTORY_NZOS_H_

namespace gfx {
  class ClientNativePixmapFactory;
}

namespace ui {

class ClientNativePixmapFactory;

// Constructor hook for use in constructor_list.cc
gfx::ClientNativePixmapFactory* CreateClientNativePixmapFactoryNzos();

}  // namespace ui

#endif  // _NZOS_CLIENT_NATIVE_PIXMAP_FACTORY_NZOS_H_
