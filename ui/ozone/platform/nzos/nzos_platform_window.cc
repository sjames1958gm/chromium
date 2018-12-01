/*
*  nzos_plaform_window.cc
*
*  Copyright (c) 2012 Netzyn Inc. All rights reserved.
*  Use of this source code maybe governed by a BSD-style license in the future.
*  Portions of this source code maybe covered by certain patents.
*
*  NzOS Application Platform Environment
*
*/

#include <string>
#include "base/command_line.h"
#include "base/files/file_path.h"
#include "base/strings/string_number_conversions.h"
#include "ui/events/platform/platform_event_source.h"
#include "ui/platform_window/platform_window_delegate.h"
#include "nzos_platform_window.h"
#include "nzos_display_browser.h"
#include "nzos_widget_constants.h"
#include "nzos_cursor_factory.h"

#include "third_party/nzos/include/QzDefines.h"
#include "third_party/nzos/include/NzApe.h"

namespace ui {

NzosPlatformWindow::NzosPlatformWindow(PlatformWindowDelegate* delegate, const gfx::Rect& bounds) : delegate_(delegate), bounds_(bounds), widget_(0) {
  NzLog("NzosPlatformWindow() being created on process with PID=%d", (uint32_t)getpid());
  static uint32_t opaque_handle = 0;
  widget_ = ++opaque_handle;
  ui::NzosDisplayBrowser::GetInstance()->AddWindow(this, widget_);
  delegate_->OnAcceleratedWidgetAvailable(widget_);
  NzLog("NzosPlatformWindow created, id:%d", widget_);
  cursor_ = QZ_CURSOR_ARROW;
}

NzosPlatformWindow::~NzosPlatformWindow() {
  ui::NzosDisplayBrowser::GetInstance()->DeleteWindow(this, widget_);
  NzLog("NzosPlatformWindow deleted, id:%d", widget_);
}

gfx::Rect NzosPlatformWindow::GetBounds() {
  return bounds_;
}

void NzosPlatformWindow::SetBounds(const gfx::Rect& bounds) {
  NzLog("SetBounds() %dx%d", bounds.width(), bounds.height());
  bounds_ = bounds;
  delegate_->OnBoundsChanged(bounds);
}

void NzosPlatformWindow::Show() {
  NzLog("NzosPlatformWindow::Show");
}

void NzosPlatformWindow::Hide() {
  NzLog("NzosPlatformWindow::Hide");
}

void NzosPlatformWindow::Close() {
  NzLog("NzosPlatformWindow::Close");
  //delegate_->OnRootWindowClosed(this);
}

void NzosPlatformWindow::PrepareForShutdown() {

}

void NzosPlatformWindow::SetTitle(const base::string16& title) {

}

void NzosPlatformWindow::SetCapture() {
}

void NzosPlatformWindow::ReleaseCapture() {
}

bool NzosPlatformWindow::HasCapture() const {
  return false;
}

void NzosPlatformWindow::ToggleFullscreen() {
}

void NzosPlatformWindow::Maximize() {
}

void NzosPlatformWindow::Minimize() {
}

void NzosPlatformWindow::Restore() {
}

PlatformWindowState NzosPlatformWindow::GetPlatformWindowState() const {
  NzLog("NzosPlatformWindow::GetPlatformWindowState");
  return PLATFORM_WINDOW_STATE_NORMAL;
}

void NzosPlatformWindow::SetCursor(PlatformCursor cursor) {
  NzLog("NzosPlatformWindow::SetCursor");
  int locCursor = QZ_CURSOR_ARROW;

  scoped_refptr<BitmapCursorOzone> nzos_cursor = ui::BitmapCursorFactoryOzone::GetBitmapCursor(cursor);
  if (nzos_cursor) {
    locCursor = NzosCursorFactory::GetPlatformCursorType(nzos_cursor->Type());
    NzLog("SetCursor to : %d", nzos_cursor->Type());
  }

  if (cursor_ != locCursor) {
    NzLogVerbose("Change cursor: %d", (uint32_t)locCursor);
    NzSetCursor(locCursor);
    cursor_ = locCursor;
  }
}

void NzosPlatformWindow::MoveCursorTo(const gfx::Point& location) {
}

void NzosPlatformWindow::ConfineCursorToBounds(const gfx::Rect& bounds) {
}

PlatformImeController* NzosPlatformWindow::GetPlatformImeController() {
  return nullptr;
}

void NzosPlatformWindow::SetRestoredBoundsInPixels(const gfx::Rect& bounds) {

}

gfx::Rect NzosPlatformWindow::GetRestoredBoundsInPixels() const {
  gfx::Rect r;
  return r;
}

}  // namespace ui
