/*
*  nzos_event_factory.h
*
*  Copyright (c) 2012 Netzyn Inc. All rights reserved.
*  Use of this source code maybe governed by a BSD-style license in the future.
*  Portions of this source code maybe covered by certain patents.
*
*  NzOS Application Platform Environment
*
*/

#ifndef __NZOS_EVENT_FACTORY_H__
#define __NZOS_EVENT_FACTORY_H__

#include "nzos_event_converter.h"

namespace ui {

class NzosWindowChangeObserver;
class NzosOutputChangeObserver;

class NzosEventFactory {
 public:
  NzosEventFactory();
  virtual ~NzosEventFactory();

  static NzosEventFactory* GetInstance();
  static void SetInstance(NzosEventFactory* instance);

  void SetWindowChangeObserver(NzosWindowChangeObserver* observer);
  NzosWindowChangeObserver* GetWindowChangeObserver() const;

  void SetOutputChangeObserver(NzosOutputChangeObserver* observer);
  NzosOutputChangeObserver* GetOutputChangeObserver() const;

  void SetEventConverter(NzosEventConverter* converter);
  NzosEventConverter* EventConverter() const;

 protected:
  NzosEventConverter*       event_converter_;
  NzosWindowChangeObserver* observer_;
  NzosOutputChangeObserver* output_observer_;
  static NzosEventFactory*  impl_;
};

}  // namespace ui

#endif  // __NZOS_EVENT_FACTORY_H__
