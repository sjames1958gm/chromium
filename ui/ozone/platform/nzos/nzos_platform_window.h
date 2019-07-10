/*
*  nzos_platform_window.h
*
*  Copyright (c) 2012 Netzyn Inc. All rights reserved.
*  Use of this source code maybe governed by a BSD-style license in the future.
*  Portions of this source code maybe covered by certain patents.
*
*  NzOS Application Platform Environment
*
*/

#ifndef __NZOS_PLATFORM_WINDOW_H__
#define __NZOS_PLATFORM_WINDOW_H__

#include "ui/gfx/geometry/rect.h"
#include "ui/gfx/native_widget_types.h"
#include "ui/platform_window/platform_window.h"

namespace ui {

class PlatformWindowDelegate;

class NzosPlatformWindow : public PlatformWindow {
 public:
  NzosPlatformWindow(PlatformWindowDelegate* delegate, const gfx::Rect& bounds);
  ~NzosPlatformWindow() override;

  // PlatformWindow overrides:
  gfx::Rect GetBounds() override;
  void SetBounds(const gfx::Rect& bounds) override;
  void Show() override;
  void Hide() override;
  void Close() override;
  void PrepareForShutdown() override;

  void SetTitle(const base::string16& title) override;

  void SetCapture() override;
  void ReleaseCapture() override;
  bool HasCapture() const override;
  void ToggleFullscreen() override;
  void Maximize() override;
  void Minimize() override;
  void Restore() override;
  PlatformWindowState GetPlatformWindowState() const override;

  void SetCursor(PlatformCursor cursor) override;
  void MoveCursorTo(const gfx::Point& location) override;
  void ConfineCursorToBounds(const gfx::Rect& bounds) override;

  PlatformWindowDelegate* Delegate() { return delegate_; }
  PlatformImeController* GetPlatformImeController() override;

  // Sets and gets the restored bounds of the platform-window.
  void SetRestoredBoundsInPixels(const gfx::Rect& bounds) override;
  gfx::Rect GetRestoredBoundsInPixels() const override;

 private:
  PlatformWindowDelegate* delegate_;
  gfx::Rect               bounds_;
  gfx::AcceleratedWidget  widget_;
  // bool                    hw_accel_;
  int                     cursor_;

  DISALLOW_COPY_AND_ASSIGN(NzosPlatformWindow);
};

}  // namespace ui

#endif  // __NZOS_PLATFORM_WINDOW_H__
