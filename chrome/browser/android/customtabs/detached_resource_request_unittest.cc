// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <memory>
#include <string>
#include <utility>

#include "base/files/file_path.h"
#include "base/run_loop.h"
#include "base/strings/string_number_conversions.h"
#include "base/test/bind_test_util.h"
#include "base/test/metrics/histogram_tester.h"
#include "chrome/browser/android/customtabs/detached_resource_request.h"
#include "chrome/test/base/testing_profile.h"
#include "content/public/browser/browser_context.h"
#include "content/public/common/referrer.h"
#include "content/public/test/browser_test_utils.h"
#include "content/public/test/test_browser_thread_bundle.h"
#include "content/public/test/test_host_resolver.h"
#include "net/dns/mock_host_resolver.h"
#include "net/test/embedded_test_server/embedded_test_server.h"
#include "net/test/embedded_test_server/http_request.h"
#include "net/test/embedded_test_server/http_response.h"
#include "net/test/embedded_test_server/request_handler_util.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "url/gurl.h"

namespace customtabs {
namespace {

using net::test_server::HttpRequest;
using net::test_server::HttpResponse;
using net::test_server::RequestQuery;

constexpr const char kSetCookieAndRedirect[] = "/set-cookie-and-redirect";
constexpr const char kSetCookieAndNoContent[] = "/set-cookie-and-no-content";
constexpr const char kHttpNoContent[] = "/nocontent";
constexpr const char kEchoTitle[] = "/echotitle";
constexpr const char kManyRedirects[] = "/many-redirects";
constexpr const char kCacheable[] = "/cachetime";
constexpr const char kLargeHeadersAndResponseSize[] =
    "/large-headers-and-response-size";

constexpr const char kCookieKey[] = "cookie";
constexpr const char kUrlKey[] = "url";
constexpr const char kCookieFromNoContent[] = "no-content-cookie";
constexpr const char kIndexKey[] = "index";
constexpr const char kMaxKey[] = "max";

const DetachedResourceRequest::Motivation kMotivation =
    DetachedResourceRequest::Motivation::kParallelRequest;

// /set-cookie-and-redirect?cookie=bla&url=https://redictected-url
// Sets a cookies, then responds with HTTP code 302.
std::unique_ptr<HttpResponse> SetCookieAndRedirect(const HttpRequest& request) {
  const GURL& url = request.GetURL();
  if (url.path() != kSetCookieAndRedirect || !url.has_query())
    return nullptr;

  auto response = std::make_unique<net::test_server::BasicHttpResponse>();
  RequestQuery query = net::test_server::ParseQuery(url);
  for (const char* key : {kCookieKey, kUrlKey}) {
    if (query.find(key) == query.end())
      return nullptr;
  }
  std::string cookie = query[kCookieKey][0];
  std::string destination = query[kUrlKey][0];

  response->AddCustomHeader("Set-Cookie", cookie);
  response->AddCustomHeader("Location", destination);
  response->set_code(net::HTTP_FOUND);
  response->set_content_type("text/html");
  response->set_content(base::StringPrintf(
      "<html><head></head><body>Redirecting to %s</body></html>",
      destination.c_str()));
  return response;
}

// /many-redirects?index=0&max=10
// Redirects a given amount of times, then responds with HTTP code 204.
std::unique_ptr<HttpResponse> ManyRedirects(const HttpRequest& request) {
  const GURL& url = request.GetURL();
  if (url.path() != kManyRedirects || !url.has_query())
    return nullptr;

  auto response = std::make_unique<net::test_server::BasicHttpResponse>();
  RequestQuery query = net::test_server::ParseQuery(url);
  int index, max;
  if (query.find(kIndexKey) == query.end() ||
      query.find(kMaxKey) == query.end() ||
      !base::StringToInt(query[kIndexKey][0], &index) ||
      !base::StringToInt(query[kMaxKey][0], &max)) {
    return nullptr;
  }

  if (index == max) {
    response->set_code(net::HTTP_NO_CONTENT);
    return response;
  }

  GURL::Replacements replacements;
  std::string new_query =
      base::StringPrintf("%s=%d&%s=%d", kIndexKey, index + 1, kMaxKey, max);
  replacements.SetQuery(new_query.c_str(),
                        url::Component(0, new_query.length()));
  GURL redirected_url = url.ReplaceComponents(replacements);

  response->AddCustomHeader("Location", redirected_url.spec());
  response->set_code(net::HTTP_FOUND);
  response->set_content_type("text/html");
  response->set_content(base::StringPrintf(
      "<html><head></head><body>Redirecting to %s</body></html>",
      redirected_url.spec().c_str()));
  return response;
}

// /set-cookie-and-no-content
// Sets a cookies, and replies with HTTP code 204.
std::unique_ptr<HttpResponse> SetCookieAndNoContent(
    const HttpRequest& request) {
  const GURL& url = request.GetURL();
  if (url.path() != kSetCookieAndNoContent)
    return nullptr;

  auto response = std::make_unique<net::test_server::BasicHttpResponse>();
  response->AddCustomHeader("Set-Cookie", kCookieFromNoContent);
  response->set_code(net::HTTP_NO_CONTENT);
  return response;
}

// /large-headers-and-response-size?10000
// Replies with large headers and a set response body size.
std::unique_ptr<HttpResponse> LargeHeadersAndResponseSize(
    const HttpRequest& request) {
  const GURL& url = request.GetURL();
  if (url.path() != kLargeHeadersAndResponseSize)
    return nullptr;

  auto response = std::make_unique<net::test_server::BasicHttpResponse>();
  response->AddCustomHeader(
      "X-Large-Header",
      std::string(DetachedResourceRequest::kMaxResponseSize, 'b'));
  response->set_code(net::HTTP_OK);

  uint32_t length;
  CHECK(base::StringToUint(request.GetURL().query(), &length));
  response->set_content(std::string(length, 'a'));
  return response;
}

// Waits for |expected_requests| requests to |path|, then reports the headers
// in |headers| and calls |closure|.
// Output parameters can be nullptr.
void WatchPathAndReportHeaders(const std::string& path,
                               int* expected_requests,
                               HttpRequest::HeaderMap* headers,
                               base::OnceClosure closure,
                               const HttpRequest& request) {
  if (request.GetURL().path() != path)
    return;
  if (expected_requests && --*expected_requests)
    return;
  if (headers)
    *headers = request.headers;
  std::move(closure).Run();
}

}  // namespace

class DetachedResourceRequestTest : public ::testing::Test {
 public:
  DetachedResourceRequestTest()
      : thread_bundle_(content::TestBrowserThreadBundle::REAL_IO_THREAD) {}
  ~DetachedResourceRequestTest() override = default;

  void SetUp() override {
    profile_ = std::make_unique<TestingProfile>();
    test_server_ = std::make_unique<net::EmbeddedTestServer>();
    embedded_test_server()->RegisterRequestHandler(
        base::BindRepeating(&SetCookieAndRedirect));
    embedded_test_server()->RegisterRequestHandler(
        base::BindRepeating(&SetCookieAndNoContent));
    embedded_test_server()->RegisterRequestHandler(
        base::BindRepeating(&ManyRedirects));
    embedded_test_server()->RegisterRequestHandler(
        base::BindRepeating(&LargeHeadersAndResponseSize));
    embedded_test_server()->AddDefaultHandlers(
        base::FilePath("chrome/test/data"));
    host_resolver_ = std::make_unique<content::TestHostResolver>();
    host_resolver_->host_resolver()->AddRule("*", "127.0.0.1");
  }

  void TearDown() override {
    profile_ = nullptr;
    base::RunLoop().RunUntilIdle();
  }

 protected:
  net::EmbeddedTestServer* embedded_test_server() const {
    return test_server_.get();
  }

  content::BrowserContext* browser_context() const { return profile_.get(); }

  void SetAndCheckCookieWithRedirect(bool third_party) {
    base::RunLoop first_request_waiter;
    base::RunLoop second_request_waiter;

    embedded_test_server()->RegisterRequestMonitor(base::BindRepeating(
        &WatchPathAndReportHeaders, kSetCookieAndRedirect, nullptr, nullptr,
        first_request_waiter.QuitClosure()));
    embedded_test_server()->RegisterRequestMonitor(
        base::BindRepeating(&WatchPathAndReportHeaders, kHttpNoContent, nullptr,
                            nullptr, second_request_waiter.QuitClosure()));
    ASSERT_TRUE(embedded_test_server()->Start());

    GURL redirected_url(embedded_test_server()->GetURL(kHttpNoContent));
    std::string relative_url =
        base::StringPrintf("%s?%s=%s&%s=%s", kSetCookieAndRedirect, kCookieKey,
                           "acookie", kUrlKey, redirected_url.spec().c_str());

    GURL url(embedded_test_server()->GetURL(relative_url));
    GURL site_for_cookies = third_party ? GURL("http://cats.google.com")
                                        : embedded_test_server()->base_url();

    std::string cookie = content::GetCookies(browser_context(), url);
    ASSERT_EQ("", cookie);

    DetachedResourceRequest::CreateAndStart(
        browser_context(), url, site_for_cookies,
        content::Referrer::GetDefaultReferrerPolicy(), kMotivation);
    first_request_waiter.Run();
    second_request_waiter.Run();

    cookie = content::GetCookies(browser_context(), url);
    ASSERT_EQ("acookie", cookie);
  }

  void SetAndCheckReferrer(const std::string& initial_referrer,
                           const std::string& expected_referrer,
                           net::URLRequest::ReferrerPolicy policy) {
    base::RunLoop request_completion_waiter;
    base::RunLoop server_request_waiter;
    HttpRequest::HeaderMap headers;

    embedded_test_server()->RegisterRequestMonitor(
        base::BindRepeating(&WatchPathAndReportHeaders, kEchoTitle, nullptr,
                            &headers, server_request_waiter.QuitClosure()));
    ASSERT_TRUE(embedded_test_server()->Start());
    GURL url(embedded_test_server()->GetURL(kEchoTitle));
    GURL site_for_cookies(initial_referrer);

    DetachedResourceRequest::CreateAndStart(
        browser_context(), url, site_for_cookies, policy, kMotivation,
        base::BindLambdaForTesting([&](int net_error) {
          EXPECT_EQ(net::OK, net_error);
          request_completion_waiter.Quit();
        }));
    server_request_waiter.Run();
    EXPECT_EQ(expected_referrer, headers["referer"]);
    request_completion_waiter.Run();
  }

 private:
  std::unique_ptr<TestingProfile> profile_;
  content::TestBrowserThreadBundle thread_bundle_;
  std::unique_ptr<content::TestHostResolver> host_resolver_;
  std::unique_ptr<net::EmbeddedTestServer> test_server_;
};

TEST_F(DetachedResourceRequestTest, Simple) {
  base::HistogramTester histogram_tester;
  base::RunLoop request_completion_waiter;
  base::RunLoop server_request_waiter;
  HttpRequest::HeaderMap headers;

  embedded_test_server()->RegisterRequestMonitor(
      base::BindRepeating(&WatchPathAndReportHeaders, kEchoTitle, nullptr,
                          &headers, server_request_waiter.QuitClosure()));
  ASSERT_TRUE(embedded_test_server()->Start());
  GURL url(embedded_test_server()->GetURL(kEchoTitle));
  GURL site_for_cookies("http://cats.google.com/");

  DetachedResourceRequest::CreateAndStart(
      browser_context(), url, site_for_cookies,
      content::Referrer::GetDefaultReferrerPolicy(), kMotivation,
      base::BindLambdaForTesting([&](int net_error) {
        EXPECT_EQ(net::OK, net_error);
        request_completion_waiter.Quit();
      }));
  server_request_waiter.Run();
  EXPECT_EQ(site_for_cookies.spec(), headers["referer"]);
  request_completion_waiter.Run();
  histogram_tester.ExpectUniqueSample(
      "CustomTabs.DetachedResourceRequest.RedirectsCount.Success", 0, 1);
  histogram_tester.ExpectTotalCount(
      "CustomTabs.DetachedResourceRequest.Duration.Success", 1);
  histogram_tester.ExpectBucketCount(
      "CustomTabs.DetachedResourceRequest.FinalStatus", net::OK, 1);
}

TEST_F(DetachedResourceRequestTest, SimpleFailure) {
  base::HistogramTester histogram_tester;
  base::RunLoop request_waiter;
  ASSERT_TRUE(embedded_test_server()->Start());
  GURL url(embedded_test_server()->GetURL("/unknown-url"));
  GURL site_for_cookies(embedded_test_server()->base_url());

  DetachedResourceRequest::CreateAndStart(
      browser_context(), url, site_for_cookies,
      content::Referrer::GetDefaultReferrerPolicy(), kMotivation,
      base::BindLambdaForTesting([&](int net_error) {
        EXPECT_NE(net::OK, net_error);
        request_waiter.Quit();
      }));
  request_waiter.Run();
  histogram_tester.ExpectUniqueSample(
      "CustomTabs.DetachedResourceRequest.RedirectsCount.Failure", 0, 1);
  histogram_tester.ExpectTotalCount(
      "CustomTabs.DetachedResourceRequest.Duration.Failure", 1);
  histogram_tester.ExpectBucketCount(
      "CustomTabs.DetachedResourceRequest.FinalStatus", -net::ERR_FAILED, 1);
}

TEST_F(DetachedResourceRequestTest, ResponseTooLarge) {
  base::HistogramTester histogram_tester;
  ASSERT_TRUE(embedded_test_server()->Start());
  GURL site_for_cookies(embedded_test_server()->base_url());

  // Checks that headers are not included in the size limit (response size is
  // 1 below the limit, hence above including headers.)
  {
    base::RunLoop request_waiter;
    GURL url(embedded_test_server()->GetURL(
        base::StringPrintf("%s?%u", kLargeHeadersAndResponseSize,
                           DetachedResourceRequest::kMaxResponseSize - 1)));

    DetachedResourceRequest::CreateAndStart(
        browser_context(), url, site_for_cookies,
        content::Referrer::GetDefaultReferrerPolicy(), kMotivation,
        base::BindLambdaForTesting([&](int net_error) {
          EXPECT_EQ(net::OK, net_error);
          request_waiter.Quit();
        }));
    request_waiter.Run();
    histogram_tester.ExpectUniqueSample(
        "CustomTabs.DetachedResourceRequest.RedirectsCount.Success", 0, 1);
    histogram_tester.ExpectTotalCount(
        "CustomTabs.DetachedResourceRequest.Duration.Success", 1);
    histogram_tester.ExpectBucketCount(
        "CustomTabs.DetachedResourceRequest.FinalStatus", net::OK, 1);
  }

  // Response too large, failure.
  {
    base::RunLoop request_waiter;
    GURL url(embedded_test_server()->GetURL(
        base::StringPrintf("%s?%u", kLargeHeadersAndResponseSize,
                           DetachedResourceRequest::kMaxResponseSize + 1)));

    DetachedResourceRequest::CreateAndStart(
        browser_context(), url, site_for_cookies,
        content::Referrer::GetDefaultReferrerPolicy(), kMotivation,
        base::BindLambdaForTesting([&](int net_error) {
          EXPECT_NE(net::OK, net_error);
          request_waiter.Quit();
        }));
    request_waiter.Run();
    histogram_tester.ExpectUniqueSample(
        "CustomTabs.DetachedResourceRequest.RedirectsCount.Failure", 0, 1);
    histogram_tester.ExpectTotalCount(
        "CustomTabs.DetachedResourceRequest.Duration.Failure", 1);
    histogram_tester.ExpectBucketCount(
        "CustomTabs.DetachedResourceRequest.FinalStatus",
        -net::ERR_INSUFFICIENT_RESOURCES, 1);
  }
}

TEST_F(DetachedResourceRequestTest, MultipleRequests) {
  base::RunLoop request_waiter;
  int expected_requests = 2;
  HttpRequest::HeaderMap headers;

  embedded_test_server()->RegisterRequestMonitor(base::BindRepeating(
      &WatchPathAndReportHeaders, kEchoTitle, &expected_requests, &headers,
      request_waiter.QuitClosure()));
  ASSERT_TRUE(embedded_test_server()->Start());

  GURL url(embedded_test_server()->GetURL(kEchoTitle));
  GURL site_for_cookies(embedded_test_server()->base_url());

  // No request coalescing, and no cache hit for a no-cache resource.
  for (int i = 0; i < 2; ++i) {
    DetachedResourceRequest::CreateAndStart(
        browser_context(), url, site_for_cookies,
        content::Referrer::GetDefaultReferrerPolicy(), kMotivation);
  }
  request_waiter.Run();
  EXPECT_EQ(site_for_cookies.spec(), headers["referer"]);
}

TEST_F(DetachedResourceRequestTest, NoReferrerWhenDowngrade) {
  base::RunLoop request_waiter;
  HttpRequest::HeaderMap headers;

  embedded_test_server()->RegisterRequestMonitor(
      base::BindRepeating(&WatchPathAndReportHeaders, kEchoTitle, nullptr,
                          &headers, request_waiter.QuitClosure()));
  ASSERT_TRUE(embedded_test_server()->Start());

  GURL url(embedded_test_server()->GetURL(kEchoTitle));
  // Downgrade, as the server is over HTTP.
  GURL site_for_cookies("https://cats.google.com");

  DetachedResourceRequest::CreateAndStart(
      browser_context(), url, site_for_cookies,
      content::Referrer::GetDefaultReferrerPolicy(), kMotivation);
  request_waiter.Run();
  EXPECT_EQ("", headers["referer"]);
}

TEST_F(DetachedResourceRequestTest, FollowRedirect) {
  base::RunLoop first_request_waiter;
  base::RunLoop second_request_waiter;

  std::string initial_relative_url =
      std::string(kSetCookieAndRedirect) + "?cookie=acookie&url=";

  embedded_test_server()->RegisterRequestMonitor(base::BindRepeating(
      &WatchPathAndReportHeaders, kSetCookieAndRedirect, nullptr, nullptr,
      first_request_waiter.QuitClosure()));
  embedded_test_server()->RegisterRequestMonitor(
      base::BindRepeating(&WatchPathAndReportHeaders, kHttpNoContent, nullptr,
                          nullptr, second_request_waiter.QuitClosure()));
  ASSERT_TRUE(embedded_test_server()->Start());

  GURL redirected_url(embedded_test_server()->GetURL(kHttpNoContent));
  GURL url(embedded_test_server()->GetURL(initial_relative_url +
                                          redirected_url.spec()));
  GURL site_for_cookies(embedded_test_server()->base_url());

  DetachedResourceRequest::CreateAndStart(
      browser_context(), url, site_for_cookies,
      content::Referrer::GetDefaultReferrerPolicy(), kMotivation);
  first_request_waiter.Run();
  second_request_waiter.Run();
}

TEST_F(DetachedResourceRequestTest, CanSetCookie) {
  SetAndCheckCookieWithRedirect(false);
}

TEST_F(DetachedResourceRequestTest, CanSetThirdPartyCookie) {
  SetAndCheckCookieWithRedirect(true);
}

TEST_F(DetachedResourceRequestTest, NoContentCanSetCookie) {
  base::RunLoop request_completion_waiter;
  ASSERT_TRUE(embedded_test_server()->Start());

  GURL url(embedded_test_server()->GetURL(kSetCookieAndNoContent));
  GURL site_for_cookies("http://cats.google.com/");

  std::string cookie = content::GetCookies(browser_context(), url);
  ASSERT_EQ("", cookie);

  DetachedResourceRequest::CreateAndStart(
      browser_context(), url, site_for_cookies,
      content::Referrer::GetDefaultReferrerPolicy(), kMotivation,
      base::BindLambdaForTesting([&](int net_error) {
        EXPECT_EQ(net::OK, net_error);
        request_completion_waiter.Quit();
      }));

  request_completion_waiter.Run();
  cookie = content::GetCookies(browser_context(), url);
  ASSERT_EQ(kCookieFromNoContent, cookie);
}

TEST_F(DetachedResourceRequestTest, DefaultReferrerPolicy) {
  // No Referrer on downgrade.
  SetAndCheckReferrer("https://cats.google.com", "",
                      content::Referrer::GetDefaultReferrerPolicy());
}

TEST_F(DetachedResourceRequestTest, OriginReferrerPolicy) {
  // Only the origin, even for downgrades.
  SetAndCheckReferrer("https://cats.google.com/cute-cats",
                      "https://cats.google.com/",
                      net::URLRequest::ReferrerPolicy::ORIGIN);
}

TEST_F(DetachedResourceRequestTest, NeverClearReferrerPolicy) {
  SetAndCheckReferrer("https://cats.google.com/cute-cats",
                      "https://cats.google.com/cute-cats",
                      net::URLRequest::ReferrerPolicy::NEVER_CLEAR_REFERRER);
}

TEST_F(DetachedResourceRequestTest, MultipleOrigins) {
  base::HistogramTester histogram_tester;
  base::RunLoop first_request_waiter;
  base::RunLoop second_request_waiter;
  base::RunLoop detached_request_waiter;

  embedded_test_server()->RegisterRequestMonitor(base::BindRepeating(
      &WatchPathAndReportHeaders, kSetCookieAndRedirect, nullptr, nullptr,
      first_request_waiter.QuitClosure()));
  embedded_test_server()->RegisterRequestMonitor(base::BindRepeating(
      &WatchPathAndReportHeaders, kSetCookieAndNoContent, nullptr, nullptr,
      second_request_waiter.QuitClosure()));
  ASSERT_TRUE(embedded_test_server()->Start());

  GURL redirected_origin("http://notgoogle.com");
  GURL redirected_url(embedded_test_server()->GetURL(redirected_origin.host(),
                                                     kSetCookieAndNoContent));
  std::string relative_url =
      base::StringPrintf("%s?%s=%s&%s=%s", kSetCookieAndRedirect, kCookieKey,
                         "acookie", kUrlKey, redirected_url.spec().c_str());

  GURL url(embedded_test_server()->GetURL(relative_url));
  GURL site_for_cookies = GURL("http://cats.google.com");

  std::string cookie = content::GetCookies(browser_context(), url);
  ASSERT_EQ("", cookie);
  cookie = content::GetCookies(browser_context(), redirected_origin);
  ASSERT_EQ("", cookie);

  DetachedResourceRequest::CreateAndStart(
      browser_context(), url, site_for_cookies,
      content::Referrer::GetDefaultReferrerPolicy(), kMotivation,
      base::BindLambdaForTesting([&](int net_error) {
        EXPECT_EQ(net::OK, net_error);
        detached_request_waiter.Quit();
      }));
  first_request_waiter.Run();
  second_request_waiter.Run();
  detached_request_waiter.Run();

  cookie = content::GetCookies(browser_context(), url);
  ASSERT_EQ("acookie", cookie);
  cookie = content::GetCookies(browser_context(), redirected_origin);
  ASSERT_EQ(kCookieFromNoContent, cookie);
  histogram_tester.ExpectUniqueSample(
      "CustomTabs.DetachedResourceRequest.RedirectsCount.Success", 1, 1);
  histogram_tester.ExpectBucketCount(
      "CustomTabs.DetachedResourceRequest.FinalStatus", net::OK, 1);
}

TEST_F(DetachedResourceRequestTest, ManyRedirects) {
  base::HistogramTester histogram_tester;
  base::RunLoop request_waiter;
  ASSERT_TRUE(embedded_test_server()->Start());

  auto relative_url = base::StringPrintf("%s?%s=%d&%s=%d", kManyRedirects,
                                         kIndexKey, 1, kMaxKey, 10);
  GURL url(embedded_test_server()->GetURL(relative_url));
  GURL site_for_cookies(embedded_test_server()->base_url());

  DetachedResourceRequest::CreateAndStart(
      browser_context(), url, site_for_cookies,
      content::Referrer::GetDefaultReferrerPolicy(), kMotivation,
      base::BindLambdaForTesting([&](int net_error) {
        EXPECT_EQ(net::OK, net_error);
        request_waiter.Quit();
      }));
  request_waiter.Run();
  histogram_tester.ExpectUniqueSample(
      "CustomTabs.DetachedResourceRequest.RedirectsCount.Success", 9, 1);
  histogram_tester.ExpectBucketCount(
      "CustomTabs.DetachedResourceRequest.FinalStatus", net::OK, 1);
}

TEST_F(DetachedResourceRequestTest, TooManyRedirects) {
  base::HistogramTester histogram_tester;
  base::RunLoop request_waiter;
  ASSERT_TRUE(embedded_test_server()->Start());

  auto relative_url = base::StringPrintf("%s?%s=%d&%s=%d", kManyRedirects,
                                         kIndexKey, 1, kMaxKey, 40);
  GURL url(embedded_test_server()->GetURL(relative_url));
  GURL site_for_cookies(embedded_test_server()->base_url());

  DetachedResourceRequest::CreateAndStart(
      browser_context(), url, site_for_cookies,
      content::Referrer::GetDefaultReferrerPolicy(), kMotivation,
      base::BindLambdaForTesting([&](int net_error) {
        EXPECT_EQ(-net::ERR_TOO_MANY_REDIRECTS, net_error);
        request_waiter.Quit();
      }));
  request_waiter.Run();
  histogram_tester.ExpectUniqueSample(
      "CustomTabs.DetachedResourceRequest.RedirectsCount.Failure", 20, 1);
  histogram_tester.ExpectBucketCount(
      "CustomTabs.DetachedResourceRequest.FinalStatus",
      -net::ERR_TOO_MANY_REDIRECTS, 1);
}

TEST_F(DetachedResourceRequestTest, CachedResponse) {
  int two_minus_requests_count = 2;
  base::RunLoop dummy_run_loop;
  embedded_test_server()->RegisterRequestMonitor(base::BindRepeating(
      &WatchPathAndReportHeaders, kCacheable, &two_minus_requests_count,
      nullptr, dummy_run_loop.QuitClosure()));

  base::RunLoop first_request_waiter;
  base::RunLoop second_request_waiter;
  ASSERT_TRUE(embedded_test_server()->Start());

  GURL url(embedded_test_server()->GetURL(kCacheable));
  GURL site_for_cookies(embedded_test_server()->base_url());

  DetachedResourceRequest::CreateAndStart(
      browser_context(), url, site_for_cookies,
      content::Referrer::GetDefaultReferrerPolicy(), kMotivation,
      base::BindLambdaForTesting([&](int net_error) {
        EXPECT_EQ(net::OK, net_error);
        first_request_waiter.Quit();
      }));
  first_request_waiter.Run();

  DetachedResourceRequest::CreateAndStart(
      browser_context(), url, site_for_cookies,
      content::Referrer::GetDefaultReferrerPolicy(), kMotivation,
      base::BindLambdaForTesting([&](int net_error) {
        EXPECT_EQ(net::OK, net_error);
        second_request_waiter.Quit();
      }));
  second_request_waiter.Run();

  // Only one request, HTTP cache hit for the second one.
  EXPECT_EQ(1, two_minus_requests_count);
}

}  // namespace customtabs
