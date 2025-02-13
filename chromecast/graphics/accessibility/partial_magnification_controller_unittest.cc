// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chromecast/graphics/accessibility/partial_magnification_controller.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "ui/aura/test/aura_test_base.h"
#include "ui/aura/test/default_event_generator_delegate.h"
#include "ui/aura/test/test_window_delegate.h"
#include "ui/display/manager/display_manager.h"
#include "ui/events/test/event_generator.h"
#include "ui/views/widget/widget.h"
#include "ui/wm/core/default_screen_position_client.h"

namespace chromecast {

class CastTestWindowDelegate : public aura::test::TestWindowDelegate {
 public:
  CastTestWindowDelegate() : key_code_(ui::VKEY_UNKNOWN) {}
  ~CastTestWindowDelegate() override {}

  // Overridden from TestWindowDelegate:
  void OnKeyEvent(ui::KeyEvent* event) override {
    key_code_ = event->key_code();
  }

  ui::KeyboardCode key_code() { return key_code_; }

 private:
  ui::KeyboardCode key_code_;

  DISALLOW_COPY_AND_ASSIGN(CastTestWindowDelegate);
};

class TestEventGeneratorDelegate
    : public aura::test::DefaultEventGeneratorDelegate {
 public:
  explicit TestEventGeneratorDelegate(aura::Window* root_window)
      : DefaultEventGeneratorDelegate(root_window), root_window_(root_window) {}
  ~TestEventGeneratorDelegate() override = default;

  // aura::test::DefaultEventGeneratorDelegate:
  aura::client::ScreenPositionClient* GetScreenPositionClient(
      const aura::Window* window) const override {
    return aura::client::GetScreenPositionClient(root_window_);
  }

 private:
  aura::Window* root_window_;

  DISALLOW_COPY_AND_ASSIGN(TestEventGeneratorDelegate);
};

// Wrapper for PartialMagnificationController that exposes internal state to
// test functions.
class PartialMagnificationControllerTestApi {
 public:
  explicit PartialMagnificationControllerTestApi(
      PartialMagnificationController* controller)
      : controller_(controller) {}
  ~PartialMagnificationControllerTestApi() = default;

  bool is_enabled() const { return controller_->is_enabled_; }
  bool is_active() const { return controller_->is_active_; }
  views::Widget* host_widget() const { return controller_->host_widget_; }

  gfx::Point GetWidgetOrigin() const {
    CHECK(host_widget());
    return host_widget()->GetWindowBoundsInScreen().origin();
  }

 private:
  PartialMagnificationController* controller_;

  DISALLOW_ASSIGN(PartialMagnificationControllerTestApi);
};

class PartialMagnificationControllerTest : public aura::test::AuraTestBase {
 public:
  PartialMagnificationControllerTest() = default;
  ~PartialMagnificationControllerTest() override = default;

  void SetUp() override {
    aura::test::AuraTestBase::SetUp();

    screen_position_client_.reset(new wm::DefaultScreenPositionClient());
    aura::client::SetScreenPositionClient(root_window(),
                                          screen_position_client_.get());
    controller_ =
        std::make_unique<PartialMagnificationController>(root_window());
  }

  void TearDown() override {
    // PartialMagnificationController needs to be deleted before the root window
    // is torn down by AuraTestBase.
    controller_.reset();

    aura::test::AuraTestBase::TearDown();
  }

 protected:
  PartialMagnificationController* GetController() const {
    return controller_.get();
  }

  PartialMagnificationControllerTestApi GetTestApi() const {
    return PartialMagnificationControllerTestApi(controller_.get());
  }

  ui::test::EventGenerator& GetEventGenerator() {
    if (!event_generator_) {
      event_generator_ = std::make_unique<ui::test::EventGenerator>(
          std::make_unique<TestEventGeneratorDelegate>(root_window()));
    }
    return *event_generator_.get();
  }

 private:
  std::unique_ptr<ui::test::EventGenerator> event_generator_;
  std::unique_ptr<aura::client::ScreenPositionClient> screen_position_client_;

  CastTestWindowDelegate test_window_delegate_;
  std::unique_ptr<PartialMagnificationController> controller_;

  DISALLOW_COPY_AND_ASSIGN(PartialMagnificationControllerTest);
};

// The magnifier should not show up immediately after being enabled.
TEST_F(PartialMagnificationControllerTest, InactiveByDefault) {
  GetController()->SetEnabled(true);
  EXPECT_FALSE(GetTestApi().is_active());
  EXPECT_EQ(GetTestApi().host_widget(), nullptr);
}

// The magnifier should show up only after a pointer is pressed while enabled.
TEST_F(PartialMagnificationControllerTest, ActiveOnPointerDown) {
  // While disabled no magnifier shows up.
  GetEventGenerator().PressTouch();
  EXPECT_FALSE(GetTestApi().is_active());
  EXPECT_FALSE(GetTestApi().host_widget());
  GetEventGenerator().ReleaseTouch();

  // While enabled the magnifier is only active while the pointer is down.
  GetController()->SetEnabled(true);
  GetEventGenerator().PressTouch();
  EXPECT_TRUE(GetTestApi().is_active());
  EXPECT_NE(GetTestApi().host_widget(), nullptr);
  GetEventGenerator().ReleaseTouch();
  EXPECT_FALSE(GetTestApi().is_active());
  EXPECT_EQ(GetTestApi().host_widget(), nullptr);
}

// Turning the magnifier off while it is active destroys the window.
TEST_F(PartialMagnificationControllerTest, DisablingDisablesActive) {
  GetController()->SetEnabled(true);
  GetEventGenerator().PressTouch();
  EXPECT_TRUE(GetTestApi().is_active());

  GetController()->SetEnabled(false);
  EXPECT_FALSE(GetTestApi().is_active());
  EXPECT_EQ(GetTestApi().host_widget(), nullptr);
}

// The magnifier is always located at finger.
TEST_F(PartialMagnificationControllerTest, MagnifierFollowsFinger) {
  GetController()->SetEnabled(true);

  // The window does not have to be centered on the press; compute the initial
  // window placement offset. Use a Vector2d for the + operator overload.
  GetEventGenerator().PressTouch();
  gfx::Vector2d offset(GetTestApi().GetWidgetOrigin().x(),
                       GetTestApi().GetWidgetOrigin().y());

  // Move the pointer around, make sure the window follows it.
  GetEventGenerator().MoveTouch(gfx::Point(32, 32));
  EXPECT_EQ(GetEventGenerator().current_location() + offset,
            GetTestApi().GetWidgetOrigin());

  GetEventGenerator().MoveTouch(gfx::Point(0, 10));
  EXPECT_EQ(GetEventGenerator().current_location() + offset,
            GetTestApi().GetWidgetOrigin());

  GetEventGenerator().MoveTouch(gfx::Point(10, 0));
  EXPECT_EQ(GetEventGenerator().current_location() + offset,
            GetTestApi().GetWidgetOrigin());

  GetEventGenerator().ReleaseTouch();

  // Make sure the window is initially placed correctly.
  GetEventGenerator().set_current_location(gfx::Point(50, 20));
  EXPECT_FALSE(GetTestApi().is_active());
  GetEventGenerator().PressTouch();
  EXPECT_EQ(GetEventGenerator().current_location() + offset,
            GetTestApi().GetWidgetOrigin());
}

}  // namespace chromecast
