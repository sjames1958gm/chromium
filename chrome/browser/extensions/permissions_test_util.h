// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_EXTENSIONS_PERMISSIONS_TEST_UTIL_H_
#define CHROME_BROWSER_EXTENSIONS_PERMISSIONS_TEST_UTIL_H_

#include "chrome/browser/extensions/permissions_updater.h"

namespace content {
class BrowserContext;
}  // namespace content

namespace extensions {

class Extension;
class PermissionSet;

namespace permissions_test_util {

// Calls corresponding PermissionsUpdater method respectively and wait for its
// asynchronous completion.
void GrantOptionalPermissionsAndWaitForCompletion(
    content::BrowserContext* browser_context,
    const Extension& extension,
    const PermissionSet& permissions);
void GrantRuntimePermissionsAndWaitForCompletion(
    content::BrowserContext* browser_context,
    const Extension& extension,
    const PermissionSet& permissions);
void RevokeOptionalPermissionsAndWaitForCompletion(
    content::BrowserContext* browser_context,
    const Extension& extension,
    const PermissionSet& permissions,
    PermissionsUpdater::RemoveType remove_type);
void RevokeRuntimePermissionsAndWaitForCompletion(
    content::BrowserContext* browser_context,
    const Extension& extension,
    const PermissionSet& permissions);

}  // namespace permissions_test_util
}  // namespace extensions

#endif  // CHROME_BROWSER_EXTENSIONS_PERMISSIONS_TEST_UTIL_H_
