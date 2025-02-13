// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/extensions/api/permissions/permissions_api.h"

#include "base/json/json_reader.h"
#include "base/json/json_writer.h"
#include "base/test/scoped_feature_list.h"
#include "chrome/browser/extensions/extension_api_unittest.h"
#include "chrome/browser/extensions/extension_function_test_utils.h"
#include "chrome/browser/extensions/extension_service.h"
#include "chrome/browser/extensions/extension_service_test_with_install.h"
#include "chrome/browser/extensions/extension_util.h"
#include "chrome/browser/extensions/permissions_test_util.h"
#include "chrome/browser/extensions/permissions_updater.h"
#include "chrome/browser/extensions/scripting_permissions_modifier.h"
#include "chrome/test/base/testing_profile.h"
#include "components/crx_file/id_util.h"
#include "extensions/common/extension_builder.h"
#include "extensions/common/extension_features.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace extensions {

namespace {

// Returns a list of |patterns| as strings, making it easy to compare for
// equality with readable errors.
// TODO(devlin): This was blatantly copy-pasted from
// scriptable_permissions_modifier_unittest.cc. Put it somewhere common.
std::vector<std::string> GetPatternsAsStrings(const URLPatternSet& patterns) {
  std::vector<std::string> pattern_strings;
  pattern_strings.reserve(patterns.size());
  for (const auto& pattern : patterns)
    pattern_strings.push_back(pattern.GetAsString());

  return pattern_strings;
}

scoped_refptr<const Extension> CreateExtensionWithPermissions(
    std::unique_ptr<base::Value> permissions,
    const std::string& name,
    bool allow_file_access) {
  int creation_flags = Extension::NO_FLAGS;
  if (allow_file_access)
    creation_flags |= Extension::ALLOW_FILE_ACCESS;
  return ExtensionBuilder()
      .SetLocation(Manifest::INTERNAL)
      .SetManifest(DictionaryBuilder()
                       .Set("name", name)
                       .Set("description", "foo")
                       .Set("manifest_version", 2)
                       .Set("version", "0.1.2.3")
                       .Set("permissions", std::move(permissions))
                       .Build())
      .AddFlags(creation_flags)
      .SetID(crx_file::id_util::GenerateId(name))
      .Build();
}

// Helper function to create a base::Value from a list of strings.
std::unique_ptr<base::Value> StringVectorToValue(
    const std::vector<std::string>& strings) {
  ListBuilder builder;
  for (const auto& str : strings)
    builder.Append(str);
  return builder.Build();
}

// Runs permissions.request() with the provided |args|, and returns the result
// of the API call. Expects the function to succeed.
// Populates |did_prompt_user| with whether the user would be prompted for the
// new permissions.
bool RunRequestFunction(
    const Extension& extension,
    Browser* browser,
    const char* args,
    std::unique_ptr<const PermissionSet>* prompted_permissions_out) {
  auto function = base::MakeRefCounted<PermissionsRequestFunction>();
  function->set_user_gesture(true);
  function->set_extension(&extension);
  std::unique_ptr<base::Value> result(
      extension_function_test_utils::RunFunctionAndReturnSingleResult(
          function.get(), args, browser, api_test_utils::NONE));
  if (!function->GetError().empty()) {
    ADD_FAILURE() << "Unexpected function error: " << function->GetError();
    return false;
  }

  if (!result || !result->is_bool()) {
    ADD_FAILURE() << "Unexpected function result.";
    return false;
  }

  *prompted_permissions_out = function->TakePromptedPermissionsForTesting();

  return result->GetBool();
}

}  // namespace

class PermissionsAPIUnitTest : public ExtensionServiceTestWithInstall {
 public:
  PermissionsAPIUnitTest() {}
  ~PermissionsAPIUnitTest() override {}
  Browser* browser() { return browser_.get(); }

  // Runs chrome.permissions.contains(|json_query|).
  bool RunContainsFunction(const std::string& manifest_permission,
                           const std::string& args_string,
                           bool allow_file_access) {
    ListBuilder required_permissions;
    required_permissions.Append(manifest_permission);
    scoped_refptr<const Extension> extension = CreateExtensionWithPermissions(
        required_permissions.Build(), "My Extension", allow_file_access);
    scoped_refptr<PermissionsContainsFunction> function(
        new PermissionsContainsFunction());
    function->set_extension(extension.get());
    bool run_result = extension_function_test_utils::RunFunction(
        function.get(), args_string, browser(), api_test_utils::NONE);
    EXPECT_TRUE(run_result) << function->GetError();

    bool has_permission;
    EXPECT_TRUE(function->GetResultList()->GetBoolean(0u, &has_permission));
    return has_permission;
  }

  // Adds the extension to the ExtensionService, and grants any inital
  // permissions.
  void AddExtensionAndGrantPermissions(const Extension& extension) {
    PermissionsUpdater updater(profile());
    updater.InitializePermissions(&extension);
    updater.GrantActivePermissions(&extension);
    service()->AddExtension(&extension);
  }

 private:
  // ExtensionServiceTestBase:
  void SetUp() override {
    ExtensionServiceTestWithInstall::SetUp();
    PermissionsRequestFunction::SetAutoConfirmForTests(true);
    InitializeEmptyExtensionService();
    browser_window_.reset(new TestBrowserWindow());
    Browser::CreateParams params(profile(), true);
    params.type = Browser::TYPE_TABBED;
    params.window = browser_window_.get();
    browser_.reset(new Browser(params));
  }
  // ExtensionServiceTestBase:
  void TearDown() override {
    browser_.reset();
    browser_window_.reset();
    PermissionsRequestFunction::ResetAutoConfirmForTests();
    ExtensionServiceTestWithInstall::TearDown();
  }

  std::unique_ptr<TestBrowserWindow> browser_window_;
  std::unique_ptr<Browser> browser_;

  DISALLOW_COPY_AND_ASSIGN(PermissionsAPIUnitTest);
};

TEST_F(PermissionsAPIUnitTest, Contains) {
  // 1. Since the extension does not have file:// origin access, expect it
  // to return false;
  bool expected_has_permission = false;
  bool has_permission = RunContainsFunction(
      "tabs", "[{\"origins\":[\"file://*\"]}]", false /* allow_file_access */);
  EXPECT_EQ(expected_has_permission, has_permission);

  // 2. Extension has tabs permission, expect to return true.
  expected_has_permission = true;
  has_permission = RunContainsFunction("tabs", "[{\"permissions\":[\"tabs\"]}]",
                                       false /* allow_file_access */);
  EXPECT_EQ(expected_has_permission, has_permission);

  // 3. Extension has file permission, but not active. Expect to return false.
  expected_has_permission = false;
  has_permission =
      RunContainsFunction("file://*", "[{\"origins\":[\"file://*\"]}]",
                          false /* allow_file_access */);
  EXPECT_EQ(expected_has_permission, has_permission);

  // 4. Same as #3, but this time with file access allowed.
  expected_has_permission = true;
  has_permission =
      RunContainsFunction("file:///*", "[{\"origins\":[\"file:///*\"]}]",
                          true /* allow_file_access */);
  EXPECT_EQ(expected_has_permission, has_permission);
}

TEST_F(PermissionsAPIUnitTest, ContainsAndGetAllWithRuntimeHostPermissions) {
  // This test relies on the click-to-script feature.
  base::test::ScopedFeatureList scoped_feature_list;
  scoped_feature_list.InitAndEnableFeature(
      extensions_features::kRuntimeHostPermissions);

  scoped_refptr<const Extension> extension =
      ExtensionBuilder("extension")
          .AddPermissions({"https://example.com/"})
          .Build();

  AddExtensionAndGrantPermissions(*extension);
  PermissionsUpdater updater(profile());
  updater.InitializePermissions(extension.get());
  updater.GrantActivePermissions(extension.get());
  service()->AddExtension(extension.get());

  auto contains_origin = [this, &extension](const char* origin) {
    auto function = base::MakeRefCounted<PermissionsContainsFunction>();
    function->set_extension(extension.get());
    if (!extension_function_test_utils::RunFunction(
            function.get(),
            base::StringPrintf(R"([{"origins": ["%s"]}])", origin), browser(),
            api_test_utils::NONE)) {
      ADD_FAILURE() << "Running function failed: " << function->GetError();
    }

    return function->GetResultList()->GetList()[0].GetBool();
  };

  auto get_all = [this, &extension]() {
    auto function = base::MakeRefCounted<PermissionsGetAllFunction>();
    function->set_extension(extension.get());

    std::vector<std::string> origins;
    if (!extension_function_test_utils::RunFunction(
            function.get(), "[]", browser(), api_test_utils::NONE)) {
      ADD_FAILURE() << "Running function failed: " << function->GetError();
      return origins;
    }

    const base::Value* results = function->GetResultList();
    if (results->GetList().size() != 1u || !results->GetList()[0].is_dict()) {
      ADD_FAILURE() << "Invalid result value";
      return origins;
    }

    const base::Value* origins_value =
        results->GetList()[0].FindKeyOfType("origins", base::Value::Type::LIST);
    for (const auto& value : origins_value->GetList())
      origins.push_back(value.GetString());

    return origins;
  };

  // Currently, the extension should have access to example.com (since
  // permissions are not withheld).
  constexpr char kExampleCom[] = "https://example.com/*";
  EXPECT_TRUE(contains_origin(kExampleCom));
  EXPECT_THAT(get_all(), testing::ElementsAre(kExampleCom));

  ScriptingPermissionsModifier modifier(profile(), extension);
  modifier.SetWithholdHostPermissions(true);

  // Once we withhold the permission, the contains function should correctly
  // report the value.
  EXPECT_FALSE(contains_origin(kExampleCom));
  EXPECT_THAT(get_all(), testing::IsEmpty());

  constexpr char kChromiumOrg[] = "https://chromium.org/";
  modifier.GrantHostPermission(GURL(kChromiumOrg));

  // The permissions API only reports active permissions, rather than granted
  // permissions. This means it will not report values for permissions that
  // aren't requested. This is probably good, because the extension wouldn't be
  // able to use them anyway (since they aren't active).
  EXPECT_FALSE(contains_origin(kChromiumOrg));
  EXPECT_THAT(get_all(), testing::IsEmpty());
}

// Tests requesting withheld permissions with the permissions.request() API.
TEST_F(PermissionsAPIUnitTest, RequestingWithheldPermissions) {
  base::test::ScopedFeatureList feature_list;
  feature_list.InitAndEnableFeature(
      extensions_features::kRuntimeHostPermissions);

  // Create an extension with required host permissions, and withhold those
  // permissions.
  scoped_refptr<const Extension> extension =
      ExtensionBuilder("extension")
          .AddPermissions({"https://example.com/*", "https://google.com/*"})
          .Build();
  AddExtensionAndGrantPermissions(*extension);
  ScriptingPermissionsModifier(profile(), extension)
      .SetWithholdHostPermissions(true);

  const GURL kExampleCom("https://example.com");
  const GURL kGoogleCom("https://google.com");
  const PermissionsData* permissions_data = extension->permissions_data();
  EXPECT_TRUE(
      permissions_data->active_permissions().effective_hosts().is_empty());

  // Request one of the withheld permissions.
  std::unique_ptr<const PermissionSet> prompted_permissions;
  EXPECT_TRUE(RunRequestFunction(*extension, browser(),
                                 R"([{"origins": ["https://example.com/*"]}])",
                                 &prompted_permissions));
  ASSERT_TRUE(prompted_permissions);
  EXPECT_THAT(GetPatternsAsStrings(prompted_permissions->effective_hosts()),
              testing::UnorderedElementsAre("https://example.com/*"));

  // The withheld permission should be granted.
  EXPECT_TRUE(
      permissions_data->active_permissions().effective_hosts().MatchesURL(
          kExampleCom));
  EXPECT_FALSE(
      permissions_data->active_permissions().effective_hosts().MatchesURL(
          kGoogleCom));
}

// Tests an extension re-requesting an optional host after the user removes it.
TEST_F(PermissionsAPIUnitTest, ReRequestingWithheldOptionalPermissions) {
  base::test::ScopedFeatureList feature_list;
  feature_list.InitAndEnableFeature(
      extensions_features::kRuntimeHostPermissions);

  // Create an extension an optional host permissions, and withhold those
  // permissions.
  scoped_refptr<const Extension> extension =
      ExtensionBuilder("extension")
          .SetManifestKey("optional_permissions",
                          StringVectorToValue({"https://chromium.org/*"}))
          .Build();
  AddExtensionAndGrantPermissions(*extension);

  const GURL kChromiumOrg("https://chromium.org");
  const PermissionsData* permissions_data = extension->permissions_data();
  EXPECT_TRUE(
      permissions_data->active_permissions().effective_hosts().is_empty());
  {
    std::unique_ptr<const PermissionSet> prompted_permissions;
    EXPECT_TRUE(RunRequestFunction(
        *extension, browser(),
        R"([{"origins": ["https://chromium.org/*"]}])", &prompted_permissions));
    ASSERT_TRUE(prompted_permissions);
    EXPECT_THAT(GetPatternsAsStrings(prompted_permissions->effective_hosts()),
                testing::UnorderedElementsAre("https://chromium.org/*"));
  }

  EXPECT_TRUE(
      permissions_data->active_permissions().effective_hosts().MatchesURL(
          kChromiumOrg));

  {
    URLPattern chromium_org_pattern(Extension::kValidHostPermissionSchemes,
                                    "https://chromium.org/*");
    PermissionSet permissions(APIPermissionSet(), ManifestPermissionSet(),
                              URLPatternSet({chromium_org_pattern}),
                              URLPatternSet());
    permissions_test_util::RevokeRuntimePermissionsAndWaitForCompletion(
        profile(), *extension, permissions);
  }
  EXPECT_TRUE(
      permissions_data->active_permissions().effective_hosts().is_empty());

  PermissionsRequestFunction::SetAutoConfirmForTests(false);
  {
    std::unique_ptr<const PermissionSet> prompted_permissions;
    EXPECT_FALSE(RunRequestFunction(
        *extension, browser(),
        R"([{"origins": ["https://chromium.org/*"]}])", &prompted_permissions));
    ASSERT_TRUE(prompted_permissions);
    EXPECT_THAT(GetPatternsAsStrings(prompted_permissions->effective_hosts()),
                testing::UnorderedElementsAre("https://chromium.org/*"));
  }
  EXPECT_TRUE(
      permissions_data->active_permissions().effective_hosts().is_empty());
}

// Tests requesting both optional and withheld permissions in the same call to
// permissions.request().
TEST_F(PermissionsAPIUnitTest, RequestingWithheldAndOptionalPermissions) {
  base::test::ScopedFeatureList feature_list;
  feature_list.InitAndEnableFeature(
      extensions_features::kRuntimeHostPermissions);

  // Create an extension with required and optional host permissions, and
  // withhold the required permissions.
  scoped_refptr<const Extension> extension =
      ExtensionBuilder("extension")
          .AddPermissions({"https://example.com/*", "https://google.com/*"})
          .SetManifestKey("optional_permissions",
                          StringVectorToValue({"https://chromium.org/*"}))
          .Build();
  AddExtensionAndGrantPermissions(*extension);
  ScriptingPermissionsModifier(profile(), extension)
      .SetWithholdHostPermissions(true);

  const GURL kExampleCom("https://example.com");
  const GURL kGoogleCom("https://google.com");
  const GURL kChromiumOrg("https://chromium.org");
  const PermissionsData* permissions_data = extension->permissions_data();
  EXPECT_TRUE(
      permissions_data->active_permissions().effective_hosts().is_empty());

  // Request one of the withheld host permissions and an optional host
  // permission in the same call.
  std::unique_ptr<const PermissionSet> prompted_permissions;
  EXPECT_TRUE(RunRequestFunction(
      *extension, browser(),
      R"([{"origins": ["https://example.com/*", "https://chromium.org/*"]}])",
      &prompted_permissions));
  ASSERT_TRUE(prompted_permissions);
  EXPECT_THAT(GetPatternsAsStrings(prompted_permissions->effective_hosts()),
              testing::UnorderedElementsAre("https://chromium.org/*",
                                            "https://example.com/*"));

  // The requested permissions should be added.
  EXPECT_TRUE(
      permissions_data->active_permissions().effective_hosts().MatchesURL(
          kExampleCom));
  EXPECT_FALSE(
      permissions_data->active_permissions().effective_hosts().MatchesURL(
          kGoogleCom));
  EXPECT_TRUE(
      permissions_data->active_permissions().effective_hosts().MatchesURL(
          kChromiumOrg));
}

// Tests requesting permissions that weren't specified in the manifest (either
// in optional permissions or in required permissions).
TEST_F(PermissionsAPIUnitTest, RequestingPermissionsNotSpecifiedInManifest) {
  base::test::ScopedFeatureList feature_list;
  feature_list.InitAndEnableFeature(
      extensions_features::kRuntimeHostPermissions);

  // Create an extension with required and optional host permissions, and
  // withhold the required permissions.
  scoped_refptr<const Extension> extension =
      ExtensionBuilder("extension")
          .AddPermissions({
              "https://example.com/*",
          })
          .SetManifestKey("optional_permissions",
                          StringVectorToValue({"https://chromium.org/*"}))
          .Build();
  AddExtensionAndGrantPermissions(*extension);
  ScriptingPermissionsModifier(profile(), extension)
      .SetWithholdHostPermissions(true);

  const GURL kExampleCom("https://example.com");
  const GURL kGoogleCom("https://google.com");
  const GURL kChromiumOrg("https://chromium.org");

  // Request permission for an optional and required permission, as well as a
  // permission that wasn't specified in the manifest. The call should fail.
  // Note: Not using RunRequestFunction(), since that expects function success.
  auto function = base::MakeRefCounted<PermissionsRequestFunction>();
  function->set_user_gesture(true);
  function->set_extension(extension.get());
  EXPECT_EQ("Only permissions specified in the manifest may be requested.",
            extension_function_test_utils::RunFunctionAndReturnError(
                function.get(),
                R"([{
               "origins": [
                 "https://example.com/*",
                 "https://chromium.org/*",
                 "https://google.com/*"
               ]
             }])",
                browser(), api_test_utils::NONE));
}

// Tests requesting withheld permissions that have already been granted.
TEST_F(PermissionsAPIUnitTest, RequestingAlreadyGrantedWithheldPermissions) {
  base::test::ScopedFeatureList feature_list;
  feature_list.InitAndEnableFeature(
      extensions_features::kRuntimeHostPermissions);

  // Create an extension with required host permissions, withhold host
  // permissions, and then grant one of the hosts.
  scoped_refptr<const Extension> extension =
      ExtensionBuilder("extension")
          .AddPermissions({"https://example.com/*", "https://google.com/*"})
          .Build();
  AddExtensionAndGrantPermissions(*extension);
  ScriptingPermissionsModifier modifier(profile(), extension);
  modifier.SetWithholdHostPermissions(true);

  const GURL kExampleCom("https://example.com");
  const GURL kGoogleCom("https://google.com");
  modifier.GrantHostPermission(kExampleCom);

  const PermissionsData* permissions_data = extension->permissions_data();
  EXPECT_TRUE(
      permissions_data->active_permissions().effective_hosts().MatchesURL(
          kExampleCom));
  EXPECT_FALSE(
      permissions_data->active_permissions().effective_hosts().MatchesURL(
          kGoogleCom));

  // Request the already-granted host permission. The function should succeed
  // (without even prompting the user), and the permission should (still) be
  // granted.
  PermissionsRequestFunction::SetAutoConfirmForTests(false);

  std::unique_ptr<const PermissionSet> prompted_permissions;
  EXPECT_TRUE(RunRequestFunction(*extension, browser(),
                                 R"([{"origins": ["https://example.com/*"]}])",
                                 &prompted_permissions));
  ASSERT_FALSE(prompted_permissions);

  // The withheld permission should be granted.
  EXPECT_TRUE(
      permissions_data->active_permissions().effective_hosts().MatchesURL(
          kExampleCom));
  EXPECT_FALSE(
      permissions_data->active_permissions().effective_hosts().MatchesURL(
          kGoogleCom));
}

}  // namespace extensions
