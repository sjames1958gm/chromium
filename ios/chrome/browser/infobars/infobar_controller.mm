// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ios/chrome/browser/infobars/infobar_controller.h"

#include <memory>

#include "base/logging.h"
#include "ios/chrome/browser/infobars/infobar_controller_delegate.h"

#if !defined(__has_feature) || !__has_feature(objc_arc)
#error "This file requires ARC support."
#endif

@interface InfoBarController () {
  UIView* _infoBarView;
}
@end

@implementation InfoBarController

@synthesize delegate = _delegate;
@synthesize infoBarDelegate = _infoBarDelegate;

#pragma mark - Public

- (instancetype)initWithInfoBarDelegate:
    (infobars::InfoBarDelegate*)infoBarDelegate {
  self = [super init];
  if (self) {
    _infoBarDelegate = infoBarDelegate;
    _infoBarView = [self infobarView];
  }
  return self;
}

- (void)dealloc {
  [_infoBarView removeFromSuperview];
}

- (UIView*)view {
  return _infoBarView;
}

- (void)removeView {
  [_infoBarView removeFromSuperview];
}

- (void)detachView {
  _delegate = nullptr;
  _infoBarDelegate = nullptr;
}

#pragma mark - Protected

- (UIView*)infobarView {
  NOTREACHED() << "Must be overriden in subclasses.";
  return _infoBarView;
}

- (BOOL)shouldIgnoreUserInteraction {
  // Ignore user interaction if view is already detached or is about to.
  return !_delegate || !_delegate->IsOwned() || !_infoBarDelegate;
}

@end
