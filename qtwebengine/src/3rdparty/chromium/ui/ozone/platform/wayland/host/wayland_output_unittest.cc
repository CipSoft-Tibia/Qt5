// Copyright 2022 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ui/ozone/platform/wayland/host/wayland_output.h"
#include "ui/ozone/platform/wayland/host/xdg_output.h"

#include "testing/gtest/include/gtest/gtest.h"
#include "ui/ozone/platform/wayland/host/wayland_output_manager.h"
#include "ui/ozone/platform/wayland/test/wayland_test.h"

using ::testing::Values;

namespace ui {

using WaylandOutputTest = WaylandTestSimple;

// Tests that name and description fall back to ones in the WaylandOutput if
// XDGOutput is not created.
TEST_F(WaylandOutputTest, NameAndDescriptionFallback) {
  constexpr char kWlOutputName[] = "kWlOutputName";
  constexpr char kWlOutputDescription[] = "kWlOutputDescription";
  constexpr char kXDGOutputName[] = "kXDGOutputName";
  constexpr char kXDGOutputDescription[] = "kXDGOutputDescription";

  auto* const output_manager = connection_->wayland_output_manager();
  ASSERT_TRUE(output_manager);

  auto* wl_output = output_manager->GetPrimaryOutput();
  ASSERT_TRUE(wl_output);
  EXPECT_FALSE(wl_output->xdg_output_);
  wl_output->name_ = kWlOutputName;
  wl_output->description_ = kWlOutputDescription;

  // We only test trivial stuff here so it is okay to create an output that is
  // not backed with a real object.
  wl_output->xdg_output_ = std::make_unique<XDGOutput>(nullptr);
  wl_output->xdg_output_->name_ = kXDGOutputName;
  wl_output->xdg_output_->description_ = kXDGOutputDescription;

  EXPECT_EQ(wl_output->name(), kXDGOutputName);
  EXPECT_EQ(wl_output->description(), kXDGOutputDescription);

  wl_output->xdg_output_.reset();

  EXPECT_EQ(wl_output->name(), kWlOutputName);
  EXPECT_EQ(wl_output->description(), kWlOutputDescription);
}

}  // namespace ui
