// Copyright 2017 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "device/gamepad/public/cpp/gamepad.h"

#include <string.h>

namespace device {

constexpr float GamepadButton::kDefaultButtonPressedThreshold;
constexpr double GamepadHapticActuator::kMaxEffectDurationMillis;
constexpr size_t Gamepad::kIdLengthCap;
constexpr size_t Gamepad::kAxesLengthCap;
constexpr size_t Gamepad::kButtonsLengthCap;

Gamepad::Gamepad()
    : connected(false),
      timestamp(0),
      axes_length(0),
      buttons_length(0),
      mapping(GamepadMapping::kNone),
      display_id(0) {
  id[0] = 0;
}

Gamepad::Gamepad(const Gamepad& other) = default;

Gamepad& Gamepad::operator=(const Gamepad& other) = default;

void Gamepad::SetID(const std::u16string& src) {
  memset(id, 0, sizeof(id));
  src.copy(id, kIdLengthCap - 1);
}

}  // namespace device
