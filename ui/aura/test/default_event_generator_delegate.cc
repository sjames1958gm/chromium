// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ui/aura/test/default_event_generator_delegate.h"

#include "ui/aura/window.h"
#include "ui/aura/window_tree_host.h"

namespace aura {
namespace test {

DefaultEventGeneratorDelegate::DefaultEventGeneratorDelegate(
    gfx::NativeWindow root_window)
    : root_window_(root_window) {}

ui::EventTarget* DefaultEventGeneratorDelegate::GetTargetAt(
    const gfx::Point& location) {
  return root_window_->GetHost()->window();
}

client::ScreenPositionClient*
DefaultEventGeneratorDelegate::GetScreenPositionClient(
    const Window* window) const {
  return nullptr;
}

}  // namespace test
}  // namespace aura
