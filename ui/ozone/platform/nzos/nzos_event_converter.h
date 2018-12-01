/*
*  nzos_event_converter.h
*
*  Copyright (c) 2012 Netzyn Inc. All rights reserved.
*  Use of this source code maybe governed by a BSD-style license in the future.
*  Portions of this source code maybe covered by certain patents.
*
*  NzOS Application Platform Environment
*
*/

#ifndef __NZOS_EVENT_CONVERTER_H__
#define __NZOS_EVENT_CONVERTER_H__

#include <string>
#include "base/message_loop/message_loop.h"
#include "ui/events/event.h"
#include "ui/events/event_constants.h"
#include "ui/platform_window/platform_window_delegate.h"

namespace ui {

class NzosWindowChangeObserver;
class NzosOutputChangeObserver;

class NzosEventConverter {
 public:
  NzosEventConverter();
  virtual ~NzosEventConverter();

  void SetWindowDelegate(PlatformWindowDelegate* delegate);

  void ButtonNotify(uint32_t op, uint32_t flags, uint32_t x, uint32_t y);
  void KeyNotify(uint32_t op, uint32_t flags, uint32_t key);
  void TouchNotify(uint64_t ts, uint32_t op, uint32_t flags, uint32_t finger, uint32_t x, uint32_t y);
  void CloseWidget(uint32_t handle);
  void WindowResized(uint32_t windowhandle, uint32_t width, uint32_t height);

  void SetWindowChangeObserver(ui::NzosWindowChangeObserver* observer);
  void SetOutputChangeObserver(ui::NzosOutputChangeObserver* observer);

 private:
  virtual void PostTaskOnMainLoop(const base::Closure& task);
  virtual void DispatchEvent(ui::Event* event);

  static void NotifyButtonPress(NzosEventConverter* data, uint32_t Op, uint32_t Flags, uint32_t X, uint32_t Y);
  static void NotifyKeyEvent(NzosEventConverter* data, uint32_t Op, uint32_t u32Flags, uint32_t Key);
  static void NotifyTouchEvent(NzosEventConverter* data, uint64_t u64EventTime, uint32_t u32Flags, uint32_t u32Op, uint32_t u32FingerId, uint32_t X, uint32_t Y);
  static void NotifyCloseWidget(NzosEventConverter* data, uint32_t handle);
  static void NotifyWindowResized(NzosEventConverter* data, uint32_t handle, uint32_t width, uint32_t height);

  static uint16_t GetCharacterCode(uint32_t code, uint32_t flags);
  static KeyboardCode GetKeyboardCode(uint32_t code);

 private:
  base::MessageLoop*            loop_;
  scoped_refptr<base::SingleThreadTaskRunner> ui_thread_runner_;
  ui::PlatformWindowDelegate*   delegate_;
  ui::NzosWindowChangeObserver* observer_;
  ui::NzosOutputChangeObserver* output_observer_;

  DISALLOW_COPY_AND_ASSIGN(NzosEventConverter);
};

}  // namespace ui

#endif  // __NZOS_EVENT_CONVERTER_H__
