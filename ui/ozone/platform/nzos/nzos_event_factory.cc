/*
*  nzos_event_factory.cc
*
*  Copyright (c) 2012 Netzyn Inc. All rights reserved.
*  Use of this source code maybe governed by a BSD-style license in the future.
*  Portions of this source code maybe covered by certain patents.
*
*  NzOS Application Platform Environment
*
*/

#include "base/bind.h"
#include "nzos_event_factory.h"
#include "nzos_window_change_observer.h"
#include "nzos_output_change_observer.h"
#include "third_party/nzos/include/NzApe.h"

namespace ui {

// static
NzosEventFactory* NzosEventFactory::impl_ = NULL;

NzosEventFactory::NzosEventFactory() : event_converter_(NULL), observer_(NULL), output_observer_(NULL) {
  LOG(ERROR) << "NzosEventFactory::SetEventConverter()";
  NzosEventFactory::SetInstance(this);
}

NzosEventFactory::~NzosEventFactory() {
  LOG(ERROR) << "NzosEventFactory::SetEventConverter()";
}

NzosEventFactory* NzosEventFactory::GetInstance() {
  if (!impl_) 
    NzLog("No NzosEventFactory implementation set");
  return impl_;
}

void NzosEventFactory::SetInstance(NzosEventFactory* impl) {
  LOG(ERROR) << "NzosEventFactory::SetEventConverter()";
  if (impl_) 
    NzLog("Replacing set NzosEventFactory implementation");
  impl_ = impl;
}

void NzosEventFactory::SetWindowChangeObserver(NzosWindowChangeObserver* observer) {
  LOG(ERROR) << "NzosEventFactory::SetEventConverter()";
  observer_ = observer;
  if (event_converter_)
    event_converter_->SetWindowChangeObserver(observer_);
}

NzosWindowChangeObserver* NzosEventFactory::GetWindowChangeObserver() const {
  return observer_;
}

void NzosEventFactory::SetOutputChangeObserver(NzosOutputChangeObserver* observer) {
  LOG(ERROR) << "NzosEventFactory::SetEventConverter()";
  output_observer_ = observer;
  if (event_converter_)
    event_converter_->SetOutputChangeObserver(output_observer_);
}

NzosOutputChangeObserver* NzosEventFactory::GetOutputChangeObserver() const {
  return output_observer_;
}

void NzosEventFactory::SetEventConverter(NzosEventConverter* converter) {
  LOG(ERROR) << "NzosEventFactory::SetEventConverter()";
  event_converter_ = converter;
}

NzosEventConverter* NzosEventFactory::EventConverter() const {
  if (!impl_) 
    NzLog("NzosEventConverter is not initialized yet");
  return event_converter_;
}

}  // namespace ui
