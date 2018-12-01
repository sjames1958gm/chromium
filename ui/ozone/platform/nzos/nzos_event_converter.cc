/*
*  nzos_event_converter.cc
*
*  Copyright (c) 2012 Netzyn Inc. All rights reserved.
*  Use of this source code maybe governed by a BSD-style license in the future.
*  Portions of this source code maybe covered by certain patents.
*
*  NzOS Application Platform Environment
*
*/

#include "base/bind.h"
#include "base/single_thread_task_runner.h"
#include "base/task_runner.h"
#include "base/threading/thread_task_runner_handle.h"
#include "base/time/time.h"
#include "ui/gfx/geometry/point.h"
#if !defined(OS_CHROMEOS)
#include "content/shell/browser/shell.h"
#endif
#include "nzos_event_converter.h"
#include "nzos_output_change_observer.h"
#include "nzos_window_change_observer.h"

#include "third_party/nzos/include/NzApe.h"

namespace ui {

NzosEventConverter::NzosEventConverter() : loop_(NULL), delegate_(NULL), observer_(NULL), output_observer_(NULL) {
  NzLog("NzosEventConverter()");
}

NzosEventConverter::~NzosEventConverter() {
  NzLog("~NzosEventConverter()");
  loop_ = NULL;
  delegate_ = NULL;
  observer_ = NULL;
  output_observer_ = NULL;
}

void NzosEventConverter::ButtonNotify(uint32_t op, uint32_t flags, uint32_t x, uint32_t y) {
  // PostTaskOnMainLoop(base::Bind(&NzosEventConverter::NotifyButtonPress, this, op, flags, x, y));
  if (ui_thread_runner_) {
    ui_thread_runner_->PostTask(
        FROM_HERE, base::BindOnce(&NzosEventConverter::NotifyButtonPress, this, op, flags, x, y));
  }
}

void NzosEventConverter::KeyNotify(uint32_t op, uint32_t flags, uint32_t key) {
  // PostTaskOnMainLoop(base::Bind(&NzosEventConverter::NotifyKeyEvent, this, op, flags, key));
  if (ui_thread_runner_) {
    ui_thread_runner_->PostTask(
        FROM_HERE, base::BindOnce(&NzosEventConverter::NotifyKeyEvent,
                                  this, op, flags, key));
  }
}

void NzosEventConverter::TouchNotify(uint64_t ts, uint32_t op, uint32_t flags, uint32_t finger, uint32_t x, uint32_t y) {
  // PostTaskOnMainLoop(base::Bind(&NzosEventConverter::NotifyTouchEvent, this, ts, op, flags, finger, x, y));
  if (ui_thread_runner_) {
    ui_thread_runner_->PostTask(
        FROM_HERE, base::BindOnce(&NzosEventConverter::NotifyTouchEvent,
                                  this, ts, op, flags, finger, x, y));
  }
}

void NzosEventConverter::CloseWidget(uint32_t handle) {
  // PostTaskOnMainLoop(base::Bind(&NzosEventConverter::NotifyCloseWidget, this, handle));
  if (ui_thread_runner_) {
    ui_thread_runner_->PostTask(
        FROM_HERE, base::BindOnce(&NzosEventConverter::NotifyCloseWidget, this, handle));
  }
}

void NzosEventConverter::WindowResized(uint32_t handle, uint32_t width, uint32_t height) {
  // PostTaskOnMainLoop(base::Bind(&NzosEventConverter::NotifyWindowResized, this, handle, width, height));
  if (ui_thread_runner_) {
    ui_thread_runner_->PostTask(
        FROM_HERE, base::BindOnce(&NzosEventConverter::NotifyWindowResized, this, handle, width, height));
  }
}

void NzosEventConverter::SetWindowChangeObserver(ui::NzosWindowChangeObserver* observer) {
  observer_ = observer;
}

void NzosEventConverter::SetOutputChangeObserver(ui::NzosOutputChangeObserver* observer) {
  output_observer_ = observer;
}

void NzosEventConverter::NotifyButtonPress(NzosEventConverter* data, uint32_t op, uint32_t flags, uint32_t x, uint32_t y) {
  ui::EventType eventType = ui::ET_UNKNOWN;
  if (op == QZ_MOUSE_OP_DOWN)
    eventType = ui::ET_MOUSE_PRESSED;
  else if (op == QZ_MOUSE_OP_UP)
    eventType = ui::ET_MOUSE_RELEASED;
  else if (op == QZ_MOUSE_OP_MOVE)
    eventType = ui::ET_MOUSE_MOVED;
  else if (op == QZ_MOUSE_OP_WHEELUP || op == QZ_MOUSE_OP_WHEELDOWN)
    eventType = ui::ET_MOUSEWHEEL;

  int eventFlags = 0;
  bool buttonChanged = false;
  int buttonChangedValue = 0;
  if (eventType == ui::ET_MOUSE_PRESSED || eventType == ui::ET_MOUSE_RELEASED) {
    buttonChanged = true;
  }
  if (flags & QZ_MOUSE_FLAGS_LEFT) {
    eventFlags |= EF_LEFT_MOUSE_BUTTON;
    if (buttonChanged) buttonChangedValue = EF_LEFT_MOUSE_BUTTON;
  }
#if 0 // Maybe not anymore ? defined(OS_CHROMEOS)
  // For Chrome OS only one button - simulate others by adding ALT/Control to left button.
  if (flags & QZ_MOUSE_FLAGS_RIGHT) {
    eventFlags |= EF_LEFT_MOUSE_BUTTON | EF_ALT_DOWN;
    if (buttonChanged) buttonChangedValue = EF_LEFT_MOUSE_BUTTON;
  }
  if (flags & QZ_MOUSE_FLAGS_MIDDLE) {
    eventFlags |= EF_LEFT_MOUSE_BUTTON | EF_CONTROL_DOWN;
    if (buttonChanged) buttonChangedValue = EF_LEFT_MOUSE_BUTTON;
  }
#else
  if (flags & QZ_MOUSE_FLAGS_RIGHT) {
    eventFlags |= EF_RIGHT_MOUSE_BUTTON;
    if (buttonChanged) buttonChangedValue = EF_RIGHT_MOUSE_BUTTON;
  }
  if (flags & QZ_MOUSE_FLAGS_MIDDLE) {
    eventFlags |= EF_MIDDLE_MOUSE_BUTTON;
    if (buttonChanged) buttonChangedValue = EF_MIDDLE_MOUSE_BUTTON;
  }
#endif
 
//  NzLog("Received EventMouse Op=%d Flags=%0x X=%d Y=%d", op, flags, x, y);
  if (eventType == ui::ET_UNKNOWN) 
    return;

  if (op != QZ_MOUSE_OP_MOVE) {
    NzLog("MOUSE eventType = %d eventFlags = %d x = %d y = %d bc = %d", eventType, eventFlags, x, y, buttonChangedValue);
  }

  gfx::Point position(x, y);
  ui::MouseEvent mouse_event(eventType, position, position, base::TimeTicks::Now(), eventFlags, buttonChangedValue);
  if (eventType == ui::ET_MOUSEWHEEL) {
    ui::MouseWheelEvent mouse_wheel_event(mouse_event, 0, (op == QZ_MOUSE_OP_WHEELUP ? MouseWheelEvent::kWheelDelta : -MouseWheelEvent::kWheelDelta));
    data->DispatchEvent(&mouse_wheel_event);
  } else {
    data->DispatchEvent(&mouse_event);
  }
}

void NzosEventConverter::NotifyKeyEvent(NzosEventConverter* data, uint32_t op, uint32_t flags, uint32_t key) {
  if (key == QZ_KEY_SHIFT_RIGHT || key == QZ_KEY_SHIFT_LEFT) return;  // For now, we eat the shift keys
  if (key == QZ_KEY_CTRL_RIGHT || key == QZ_KEY_CTRL_LEFT) return;    // For now, we eat the ctrl keys
  if (key == QZ_KEY_NUMLOCK) return;                                  // For now, we eat the num lock key

#if !defined(OS_CHROMEOS)
  // Since we are running in the content_shell which does not have the navigation bar, we handle the following events
  // BACK FORWARD REFRESH STOP
  if (key == QZ_KEY_BACK) {
    if (op == QZ_KEY_OP_DOWN)
      content::Shell::windows()[0]->GoBackOrForward(-1);
    return;
  }
  if (key == QZ_KEY_FORWARD) {
    if (op == QZ_KEY_OP_DOWN)
      content::Shell::windows()[0]->GoBackOrForward(1);
    return;
  }
  if (key == QZ_KEY_RELOAD) {
    if (op == QZ_KEY_OP_DOWN)
      content::Shell::windows()[0]->Reload();
    return;
  }
  if (key == QZ_KEY_STOP) {
    if (op == QZ_KEY_OP_DOWN)
      content::Shell::windows()[0]->Stop();
    return;
  }
#endif

  int eventFlags = ui::EF_NONE;
  // TODOSJ
  // if (flags & QZ_KEY_FLAGS_CAPSLOCK)
  //   eventFlags |= ui::EF_CAPS_LOCK_DOWN;
  if ((flags & QZ_KEY_FLAGS_ALT_RIGHT) || (flags & QZ_KEY_FLAGS_ALT_LEFT))
    eventFlags |= ui::EF_ALT_DOWN;
  if ((flags & QZ_KEY_FLAGS_CTRL_RIGHT) || (flags & QZ_KEY_FLAGS_CTRL_LEFT))
    eventFlags |= ui::EF_CONTROL_DOWN;
  if ((flags & QZ_KEY_FLAGS_SHIFT_RIGHT) || (flags & QZ_KEY_FLAGS_SHIFT_LEFT))
    eventFlags |= ui::EF_SHIFT_DOWN;
  if (key >= QZ_KEY_NUMPAD0 && key <= QZ_KEY_NUMPAD_ENTER)
  {
    // Special handling for NUMPAD keys since content_shell does not like our key transactions
    //eventFlags |= ui::EF_NUMPAD_KEY;
    bool numlock = (flags & QZ_KEY_FLAGS_NUMLOCK);
    switch (key) {
      case QZ_KEY_NUMPAD0:        { if (numlock) key = '0'; else key = QZ_KEY_INSERT; break; }
      case QZ_KEY_NUMPAD1:        { if (numlock) key = '1'; else key = QZ_KEY_END; break; }
      case QZ_KEY_NUMPAD2:        { if (numlock) key = '2'; else key = QZ_KEY_DOWN; break; }
      case QZ_KEY_NUMPAD3:        { if (numlock) key = '3'; else key = QZ_KEY_PAGE_DOWN; break; }
      case QZ_KEY_NUMPAD4:        { if (numlock) key = '4'; else key = QZ_KEY_LEFT; break; }
      case QZ_KEY_NUMPAD5:        { if (numlock) key = '5'; else key = VKEY_UNKNOWN; break; }
      case QZ_KEY_NUMPAD6:        { if (numlock) key = '6'; else key = QZ_KEY_RIGHT; break; }
      case QZ_KEY_NUMPAD7:        { if (numlock) key = '7'; else key = QZ_KEY_HOME; break; }
      case QZ_KEY_NUMPAD8:        { if (numlock) key = '8'; else key = QZ_KEY_UP; break; }
      case QZ_KEY_NUMPAD9:        { if (numlock) key = '9'; else key = QZ_KEY_PAGE_UP; break; }
      case QZ_KEY_NUMPAD_MULTIPLY:{ key = '*'; break; }
      case QZ_KEY_NUMPAD_PLUS:    { key = '+'; break; }
      case QZ_KEY_NUMPAD_MINUS:   { key = '-'; break; }
      case QZ_KEY_NUMPAD_DECIMAL: { key = '.'; break; }
      case QZ_KEY_NUMPAD_DIVIDE:  { key = '/'; break; }
      case QZ_KEY_NUMPAD_ENTER:   { key = QZ_KEY_ENTER; break; }
    }
  }

  ui::EventType eventType = (op == QZ_KEY_OP_DOWN ? ET_KEY_PRESSED : ET_KEY_RELEASED);
  ui::KeyboardCode code = NzosEventConverter::GetKeyboardCode(key);
  uint16_t charcode = NzosEventConverter::GetCharacterCode(key, flags);

  // NzLog("KEYBOARD type = %s keycode = %04X charcode = %04X flags = %0x", (eventType==ET_KEY_PRESSED?"PRESSED":"RELEASED"), (uint32_t)code, (uint32_t)charcode, (uint32_t)eventFlags);
  KeyEvent key_event(eventType, code, eventFlags);
  key_event.set_character(charcode);

  data->DispatchEvent(&key_event);
}

void NzosEventConverter::NotifyTouchEvent(NzosEventConverter* data, uint64_t ts, uint32_t op, uint32_t flags, uint32_t finger, uint32_t x, uint32_t y) {
  /* From QzMsgTouch.h
    e_QzMsgTouch_Op_FingerMove,
    e_QzMsgTouch_Op_FingerDown,
    e_QzMsgTouch_Op_FingerUp,
    e_QzMsgTouch_Op_TapBegin,           //  One or more fingers are down
    e_QzMsgTouch_Op_TapEnd,             //  A finger came up after TapBegin
    e_QzMsgTouch_Op_TapLong,            //  One or more tap fingers down for long time (2 sec)
    e_QzMsgTouch_Op_SetHook,            //  Parent will hook touch clicks in rectangle
    e_QzMsgTouch_Op_ClearHook,          //  Parent will no longer hook touch clicks in rectangle

    #define c_QzMsgTouch_Flags_StartOfTouch     0x00000001
    #define c_QzMsgTouch_Flags_EndOfTouch       0x00000002
  */
  const char* eventName = 0;
  ui::EventType eventType; 
  if (op == QZ_TOUCH_FINGER_MOVE) {
    eventType = ET_TOUCH_MOVED;
    eventName = "ET_TOUCH_MOVED";
  }
  else if (op == QZ_TOUCH_FINGER_DOWN)
  {
    eventType = ET_TOUCH_PRESSED;
    eventName = "ET_TOUCH_PRESSED";
  }
  else if (op == QZ_TOUCH_FINGER_UP)
  {
    eventType = ET_TOUCH_RELEASED;
    eventName = "ET_TOUCH_RELEASED";
  }
  else
  {
//    LOG(ERROR) << "Touch Event ignored, op = " << op;
    return;
  }
  LOG(INFO) << "Touch Event: " << eventName << ", Finger: " << finger << " (" << x << "/" << y << ")";

  gfx::Point position(x, y);
  // TODOSJ
  // TouchEvent touch_event(eventType, position, finger, base::Time::FromInternalValue(ts));
  // touch_event.set_may_cause_scrolling(true);
  // data->DispatchEvent(&touch_event); 
}

void NzosEventConverter::NotifyCloseWidget(NzosEventConverter* data, uint32_t handle) {
  if (data->observer_)
    data->observer_->OnWindowClose(handle);
}

void NzosEventConverter::NotifyWindowResized(NzosEventConverter* data, uint32_t handle, uint32_t width, uint32_t height) {
  NzLog("NzosEventConverter::NotifyWindowResized() %dx%d", width, height);
  if (data->observer_)
    data->observer_->OnWindowResized(handle, width, height);
}

uint16_t NzosEventConverter::GetCharacterCode(uint32_t code, uint32_t flags) {
  bool ctrl  = ((flags & QZ_KEY_FLAGS_CTRL_RIGHT) || (flags & QZ_KEY_FLAGS_CTRL_LEFT));
  bool shift = ((flags & QZ_KEY_FLAGS_SHIFT_RIGHT) || (flags & QZ_KEY_FLAGS_SHIFT_LEFT));

  // Other ctrl characters
  if (ctrl) {
    uint32_t charcode = code;
    if (charcode >= 'a' && charcode <= 'z')
      charcode = 'A' + (charcode - 'a');

    if (charcode >= 'A' && charcode <= 'Z')
      return charcode - 'A' + 1;

    if (shift) {
      // following graphics chars require shift key to input.
      switch (charcode) {
        case '@': return 0x0000;  // ctrl @ maps to \x00 (Null byte)
        case '^': return 0x001E;  // ctrl ^ maps to \x1E (Record separator, Information separator two)
        case '-': return 0x001F;  // ctrl - maps to \x1F (Unit separator, Information separator one)
        case '_': return 0x001F;  // ctrl _ maps to \x1F (Unit separator, Information separator one)
        default:  return 0x0000;
      }
    } else {
      switch (charcode) {
        case '['   : return 0x001B; // ctrl [ maps to \x1B (Escape)
        case '/'   : return 0x001C; // ctrl \ maps to \x1C (File separator, Information separator four)
        case ']'   : return 0x001D; // ctrl ] maps to \x1D (Group separator, Information separator three)
        case 0x000D: return 0x000A; // ctrl Enter maps to \x0A (Line feed)
        default: return 0x0000;
      }
    }
  }

  if ((code >= 'a' && code <= 'z') ||
      (code >= 'A' && code <= 'Z') ||
      (code >= '0' && code <= '9'))
    return code;

  switch (code) {
    case QZ_KEY_TAB:    return 0x0009;
    case QZ_KEY_ESC:    return 0x001B;
    case QZ_KEY_RETURN: return 0x000D;
    case QZ_KEY_SPACE:  return 0x0020;
    case QZ_KEY_BACK:   return 0x0008;
    default:            return code;
  }
}

ui::KeyboardCode NzosEventConverter::GetKeyboardCode(uint32_t code) {

  // Implementation inspired by ozone-wayand
  if (code >= 'a' && code <= 'z')
    return static_cast<ui::KeyboardCode>('A' + (code - 'a'));
  if (code >= 'A' && code <= 'Z')
    return static_cast<ui::KeyboardCode>(code);
  if (code >= '0' && code <= '9')
    return static_cast<ui::KeyboardCode>(code);
  if (code < 0x1C)
    return ui::VKEY_UNKNOWN;

  switch (code) {
    case QZ_KEY_BACK:              return ui::VKEY_BACK;
    case QZ_KEY_TAB:               return ui::VKEY_TAB;
    case QZ_KEY_CLEAR:             return ui::VKEY_CLEAR;
    case QZ_KEY_SPACE:             return ui::VKEY_SPACE;
    case QZ_KEY_BACKSPACE:         return ui::VKEY_BACK;
    case QZ_KEY_ENTER:             return ui::VKEY_RETURN;
    case QZ_KEY_PAUSE:             return ui::VKEY_PAUSE;
    case QZ_KEY_ESC:               return ui::VKEY_ESCAPE;
    case QZ_KEY_PAGE_UP:           return ui::VKEY_PRIOR;
    case QZ_KEY_PAGE_DOWN:         return ui::VKEY_NEXT;
    case QZ_KEY_END:               return ui::VKEY_END;
    case QZ_KEY_HOME:              return ui::VKEY_HOME;
    case QZ_KEY_LEFT:              return ui::VKEY_LEFT;
    case QZ_KEY_UP:                return ui::VKEY_UP;
    case QZ_KEY_RIGHT:             return ui::VKEY_RIGHT;
    case QZ_KEY_DOWN:              return ui::VKEY_DOWN;
    case QZ_KEY_PRINT_SCREEN:      return ui::VKEY_PRINT;
    case QZ_KEY_INSERT:            return ui::VKEY_INSERT;
    case QZ_KEY_DELETE:            return ui::VKEY_DELETE;
    case QZ_KEY_HELP:              return ui::VKEY_HELP;
    case QZ_KEY_NUMLOCK:           return ui::VKEY_NUMLOCK;
    case QZ_KEY_CAPSLOCK:          return ui::VKEY_CAPITAL;
    case QZ_KEY_SCROLLOCK:         return ui::VKEY_SCROLL;
    case QZ_KEY_SHIFT_RIGHT:       return ui::VKEY_RSHIFT;
    case QZ_KEY_SHIFT_LEFT:        return ui::VKEY_LSHIFT;
    case QZ_KEY_CTRL_RIGHT:        return ui::VKEY_RCONTROL;
    case QZ_KEY_CTRL_LEFT:         return ui::VKEY_LCONTROL;
    case QZ_KEY_ALT_RIGHT:         return ui::VKEY_RMENU;
    case QZ_KEY_ALT_LEFT:          return ui::VKEY_LMENU;
    case QZ_KEY_NUMPAD0:           return ui::VKEY_NUMPAD0;
    case QZ_KEY_NUMPAD1:           return ui::VKEY_NUMPAD1;
    case QZ_KEY_NUMPAD2:           return ui::VKEY_NUMPAD2;
    case QZ_KEY_NUMPAD3:           return ui::VKEY_NUMPAD3;
    case QZ_KEY_NUMPAD4:           return ui::VKEY_NUMPAD4;
    case QZ_KEY_NUMPAD5:           return ui::VKEY_NUMPAD5;
    case QZ_KEY_NUMPAD6:           return ui::VKEY_NUMPAD6;
    case QZ_KEY_NUMPAD7:           return ui::VKEY_NUMPAD7;
    case QZ_KEY_NUMPAD8:           return ui::VKEY_NUMPAD8;
    case QZ_KEY_NUMPAD9:           return ui::VKEY_NUMPAD9;
    case QZ_KEY_NUMPAD_MULTIPLY:   return ui::VKEY_MULTIPLY;
    case QZ_KEY_NUMPAD_PLUS:       return ui::VKEY_OEM_PLUS;
    case QZ_KEY_NUMPAD_MINUS:      return ui::VKEY_OEM_MINUS;
    case QZ_KEY_NUMPAD_DECIMAL:    return ui::VKEY_DECIMAL;
    case QZ_KEY_NUMPAD_DIVIDE:     return ui::VKEY_DIVIDE;
    case QZ_KEY_NUMPAD_ENTER:      return ui::VKEY_RETURN;
    case QZ_KEY_F1:                return ui::VKEY_F1;
    case QZ_KEY_F2:                return ui::VKEY_F2;
    case QZ_KEY_F3:                return ui::VKEY_F3;
    case QZ_KEY_F4:                return ui::VKEY_F4;
    case QZ_KEY_F5:                return ui::VKEY_F5;
    case QZ_KEY_F6:                return ui::VKEY_F6;
    case QZ_KEY_F7:                return ui::VKEY_F7;
    case QZ_KEY_F8:                return ui::VKEY_F8;
    case QZ_KEY_F9:                return ui::VKEY_F9;
    case QZ_KEY_F10:               return ui::VKEY_F10;
    case QZ_KEY_F11:               return ui::VKEY_F11;
    case QZ_KEY_F12:               return ui::VKEY_F12;
    case QZ_KEY_F13:               return ui::VKEY_F13;
    case QZ_KEY_F14:               return ui::VKEY_F14;
    case QZ_KEY_F15:               return ui::VKEY_F15;
    case QZ_KEY_MENU:              return ui::VKEY_MENU;
    case QZ_KEY_VOLUME_UP:         return ui::VKEY_VOLUME_UP;
    case QZ_KEY_VOLUME_DOWN:       return ui::VKEY_VOLUME_DOWN;
    case QZ_KEY_ALTGR:             return ui::VKEY_ALTGR;
    case QZ_KEY_PLAY:              return ui::VKEY_PLAY;
    case QZ_KEY_SELECT:            return ui::VKEY_SELECT;
    case QZ_KEY_ZOOM:              return ui::VKEY_ZOOM;
    default:                       return ui::VKEY_OEM_102;
  }
}

void NzosEventConverter::PostTaskOnMainLoop(const base::Closure& task) {
  if (loop_) {
    loop_->RunTask(new base::PendingTask(FROM_HERE, task));
  }
  else
    NzLog("Unable to post task on the main loop");
}

void NzosEventConverter::SetWindowDelegate(PlatformWindowDelegate* delegate) {
  NzLog("Window delegate and main loop set. Event processing is ENABLED");
  if (!delegate_)
    delegate_ = delegate;
  if (!loop_)
    loop_ = base::MessageLoop::current();
  if (!ui_thread_runner_)
    ui_thread_runner_ = base::ThreadTaskRunnerHandle::Get();
}

void NzosEventConverter::DispatchEvent(ui::Event* event) {
  if (delegate_)
    delegate_->DispatchEvent(event);
  else
    NzLog("Unable to dispatch event");
}

}  // namespace ui
