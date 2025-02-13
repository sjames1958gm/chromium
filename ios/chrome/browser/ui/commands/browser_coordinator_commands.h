// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef IOS_CHROME_BROWSER_UI_COMMANDS_BROWSER_COORDINATOR_COMMANDS_H_
#define IOS_CHROME_BROWSER_UI_COMMANDS_BROWSER_COORDINATOR_COMMANDS_H_

#import <Foundation/Foundation.h>

// Protocol for commands that will be handled by the BrowserCoordinator.
// TODO(crbug.com/906662) : Rename this protocol to one that is more descriptive
// and representative of the contents.
@protocol BrowserCoordinatorCommands

// Shows the consent bump if it is required.
- (void)showConsentBumpIfNeeded;

// Shows the Reading List UI.
- (void)showReadingList;

// Shows recent tabs.
- (void)showRecentTabs;

@end

#endif  // IOS_CHROME_BROWSER_UI_COMMANDS_BROWSER_COORDINATOR_COMMANDS_H_
