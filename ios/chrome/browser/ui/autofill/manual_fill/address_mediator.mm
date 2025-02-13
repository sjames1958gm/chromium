// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "ios/chrome/browser/ui/autofill/manual_fill/address_mediator.h"

#include <vector>

#include "base/metrics/user_metrics.h"
#include "components/autofill/core/browser/autofill_profile.h"
#import "ios/chrome/browser/ui/autofill/manual_fill/action_cell.h"
#import "ios/chrome/browser/ui/autofill/manual_fill/address.h"
#import "ios/chrome/browser/ui/autofill/manual_fill/address_consumer.h"
#import "ios/chrome/browser/ui/autofill/manual_fill/address_form.h"
#import "ios/chrome/browser/ui/autofill/manual_fill/address_list_delegate.h"
#import "ios/chrome/browser/ui/autofill/manual_fill/manual_fill_address_cell.h"
#import "ios/chrome/browser/ui/autofill/manual_fill/manual_fill_content_delegate.h"
#import "ios/chrome/browser/ui/list_model/list_model.h"
#import "ios/chrome/browser/ui/table_view/table_view_model.h"
#include "ios/chrome/grit/ios_strings.h"
#include "ui/base/l10n/l10n_util_mac.h"

#if !defined(__has_feature) || !__has_feature(objc_arc)
#error "This file requires ARC support."
#endif

namespace manual_fill {
NSString* const ManageAddressAccessibilityIdentifier =
    @"kManualFillManageAddressAccessibilityIdentifier";
}  // namespace manual_fill

@interface ManualFillAddressMediator ()

// All available addresses.
@property(nonatomic, strong) NSArray<ManualFillAddress*>* addresses;

@end

@implementation ManualFillAddressMediator

- (instancetype)initWithProfiles:
    (std::vector<autofill::AutofillProfile*>)profiles {
  self = [super init];
  if (self) {
    NSMutableArray<ManualFillAddress*>* manualFillAddresses =
        [[NSMutableArray alloc] initWithCapacity:profiles.size()];
    for (autofill::AutofillProfile* profile : profiles) {
      [manualFillAddresses
          addObject:[[ManualFillAddress alloc] initWithProfile:*profile]];
    }
    _addresses = manualFillAddresses;
  }
  return self;
}

- (void)setConsumer:(id<ManualFillAddressConsumer>)consumer {
  if (consumer == _consumer) {
    return;
  }
  _consumer = consumer;
  [self postAddressesToConsumer];
  [self postActionsToConsumer];
}

#pragma mark - Private

// Posts the addresses to the consumer.
- (void)postAddressesToConsumer {
  if (!self.consumer) {
    return;
  }

  NSMutableArray* items =
      [[NSMutableArray alloc] initWithCapacity:self.addresses.count];
  for (ManualFillAddress* address in self.addresses) {
    auto item =
        [[ManualFillAddressItem alloc] initWithAddress:address
                                              delegate:self.contentDelegate];
    [items addObject:item];
  }

  [self.consumer presentAddresses:items];
}

- (void)postActionsToConsumer {
  if (!self.consumer) {
    return;
  }
  NSString* manageAddressesTitle =
      l10n_util::GetNSString(IDS_IOS_MANUAL_FALLBACK_MANAGE_ADDRESSES);
  __weak __typeof(self) weakSelf = self;
  auto manageAddressesItem = [[ManualFillActionItem alloc]
      initWithTitle:manageAddressesTitle
             action:^{
               base::RecordAction(base::UserMetricsAction(
                   "ManualFallback_Profiles_OpenManageProfiles"));
               [weakSelf.navigationDelegate openAddressSettings];
             }];
  manageAddressesItem.accessibilityIdentifier =
      manual_fill::ManageAddressAccessibilityIdentifier;
  [self.consumer presentActions:@[ manageAddressesItem ]];
}

@end
