// Copyright 2020 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/ui/webui/settings/ash/device_section.h"

#include "ash/constants/ash_features.h"
#include "ash/constants/ash_switches.h"
#include "ash/public/ash_interfaces.h"
#include "ash/public/cpp/night_light_controller.h"
#include "ash/public/cpp/stylus_utils.h"
#include "ash/shell.h"
#include "ash/system/power/adaptive_charging_controller.h"
#include "ash/system/power/battery_saver_controller.h"
#include "base/command_line.h"
#include "base/feature_list.h"
#include "base/metrics/histogram_functions.h"
#include "base/no_destructor.h"
#include "base/strings/string_number_conversions.h"
#include "base/strings/utf_string_conversions.h"
#include "chrome/browser/ash/drive/file_system_util.h"
#include "chrome/browser/ash/login/demo_mode/demo_session.h"
#include "chrome/browser/ui/webui/ash/settings/search/search_tag_registry.h"
#include "chrome/browser/ui/webui/settings/ash/device_display_handler.h"
#include "chrome/browser/ui/webui/settings/ash/device_keyboard_handler.h"
#include "chrome/browser/ui/webui/settings/ash/device_pointer_handler.h"
#include "chrome/browser/ui/webui/settings/ash/device_power_handler.h"
#include "chrome/browser/ui/webui/settings/ash/device_stylus_handler.h"
#include "chrome/browser/ui/webui/settings/ash/os_settings_features_util.h"
#include "chrome/browser/ui/webui/webui_util.h"
#include "chrome/common/chrome_features.h"
#include "chrome/common/url_constants.h"
#include "chrome/common/webui_url_constants.h"
#include "chrome/grit/generated_resources.h"
#include "components/user_manager/user_manager.h"
#include "content/public/browser/web_ui.h"
#include "content/public/browser/web_ui_data_source.h"
#include "media/base/media_switches.h"
#include "ui/base/l10n/l10n_util.h"
#include "ui/base/webui/web_ui_util.h"
#include "ui/display/display_features.h"
#include "ui/display/display_switches.h"
#include "ui/display/manager/touch_device_manager.h"
#include "ui/events/ash/keyboard_capability.h"
#include "ui/events/ash/keyboard_layout_util.h"
#include "ui/events/devices/device_data_manager.h"

namespace ash::settings {

namespace mojom {
using ::chromeos::settings::mojom::kAudioSubpagePath;
using ::chromeos::settings::mojom::kCustomizeMouseButtonsSubpagePath;
using ::chromeos::settings::mojom::kCustomizePenButtonsSubpagePath;
using ::chromeos::settings::mojom::kCustomizeTabletButtonsSubpagePath;
using ::chromeos::settings::mojom::kDeviceSectionPath;
using ::chromeos::settings::mojom::kDisplaySubpagePath;
using ::chromeos::settings::mojom::kExternalStorageSubpagePath;
using ::chromeos::settings::mojom::kGraphicsTabletSubpagePath;
using ::chromeos::settings::mojom::kKeyboardSubpagePath;
using ::chromeos::settings::mojom::kPerDeviceKeyboardRemapKeysSubpagePath;
using ::chromeos::settings::mojom::kPerDeviceKeyboardSubpagePath;
using ::chromeos::settings::mojom::kPerDeviceMouseSubpagePath;
using ::chromeos::settings::mojom::kPerDevicePointingStickSubpagePath;
using ::chromeos::settings::mojom::kPerDeviceTouchpadSubpagePath;
using ::chromeos::settings::mojom::kPointersSubpagePath;
using ::chromeos::settings::mojom::kPowerSubpagePath;
using ::chromeos::settings::mojom::kStorageSubpagePath;
using ::chromeos::settings::mojom::kStylusSubpagePath;
using ::chromeos::settings::mojom::Section;
using ::chromeos::settings::mojom::Setting;
using ::chromeos::settings::mojom::Subpage;
}  // namespace mojom

namespace {

const std::vector<SearchConcept>& GetDeviceSearchConcepts() {
  static const base::NoDestructor<std::vector<SearchConcept>> tags({
      {IDS_OS_SETTINGS_TAG_POWER,
       mojom::kPowerSubpagePath,
       mojom::SearchResultIcon::kPower,
       mojom::SearchResultDefaultRank::kMedium,
       mojom::SearchResultType::kSubpage,
       {.subpage = mojom::Subpage::kPower}},
      {IDS_OS_SETTINGS_TAG_DISPLAY_SIZE,
       mojom::kDisplaySubpagePath,
       mojom::SearchResultIcon::kDisplay,
       mojom::SearchResultDefaultRank::kMedium,
       mojom::SearchResultType::kSetting,
       {.setting = mojom::Setting::kDisplaySize},
       {
           IDS_OS_SETTINGS_TAG_DISPLAY_SIZE_ALT1,
           IDS_OS_SETTINGS_TAG_DISPLAY_SIZE_ALT2,
           IDS_OS_SETTINGS_TAG_DISPLAY_SIZE_ALT3,
           IDS_OS_SETTINGS_TAG_DISPLAY_SIZE_ALT4,
           IDS_OS_SETTINGS_TAG_DISPLAY_SIZE_ALT5,
       }},
      {IDS_OS_SETTINGS_TAG_STORAGE,
       mojom::kStorageSubpagePath,
       mojom::SearchResultIcon::kHardDrive,
       mojom::SearchResultDefaultRank::kMedium,
       mojom::SearchResultType::kSubpage,
       {.subpage = mojom::Subpage::kStorage},
       {IDS_OS_SETTINGS_TAG_STORAGE_ALT1, IDS_OS_SETTINGS_TAG_STORAGE_ALT2,
        SearchConcept::kAltTagEnd}},
      {IDS_OS_SETTINGS_TAG_DISPLAY_NIGHT_LIGHT,
       mojom::kDisplaySubpagePath,
       mojom::SearchResultIcon::kDisplay,
       mojom::SearchResultDefaultRank::kLow,
       mojom::SearchResultType::kSetting,
       {.setting = mojom::Setting::kNightLight},
       {IDS_OS_SETTINGS_TAG_DISPLAY_NIGHT_LIGHT_ALT1,
        IDS_OS_SETTINGS_TAG_DISPLAY_NIGHT_LIGHT_ALT2,
        SearchConcept::kAltTagEnd}},
      {IDS_OS_SETTINGS_TAG_DISPLAY,
       mojom::kDisplaySubpagePath,
       mojom::SearchResultIcon::kDisplay,
       mojom::SearchResultDefaultRank::kMedium,
       mojom::SearchResultType::kSubpage,
       {.subpage = mojom::Subpage::kDisplay},
       {IDS_OS_SETTINGS_TAG_DISPLAY_ALT1, IDS_OS_SETTINGS_TAG_DISPLAY_ALT2,
        SearchConcept::kAltTagEnd}},
      {IDS_OS_SETTINGS_TAG_DEVICE,
       mojom::kDeviceSectionPath,
       mojom::SearchResultIcon::kLaptop,
       mojom::SearchResultDefaultRank::kHigh,
       mojom::SearchResultType::kSection,
       {.section = mojom::Section::kDevice}},
      {IDS_OS_SETTINGS_TAG_POWER_IDLE_WHILE_CHARGING,
       mojom::kPowerSubpagePath,
       mojom::SearchResultIcon::kPower,
       mojom::SearchResultDefaultRank::kMedium,
       mojom::SearchResultType::kSetting,
       {.setting = mojom::Setting::kPowerIdleBehaviorWhileCharging},
       {IDS_OS_SETTINGS_TAG_POWER_IDLE_WHILE_CHARGING_ALT1,
        SearchConcept::kAltTagEnd}},
      {IDS_OS_SETTINGS_TAG_POWER_IDLE_WHILE_ON_BATTERY,
       mojom::kPowerSubpagePath,
       mojom::SearchResultIcon::kPower,
       mojom::SearchResultDefaultRank::kMedium,
       mojom::SearchResultType::kSetting,
       {.setting = mojom::Setting::kPowerIdleBehaviorWhileOnBattery},
       {IDS_OS_SETTINGS_TAG_POWER_IDLE_WHILE_ON_BATTERY_ALT1,
        SearchConcept::kAltTagEnd}},
      {IDS_OS_SETTINGS_TAG_AUDIO_SETTINGS,
       mojom::kAudioSubpagePath,
       mojom::SearchResultIcon::kAudio,
       mojom::SearchResultDefaultRank::kMedium,
       mojom::SearchResultType::kSubpage,
       {.subpage = mojom::Subpage::kAudio},
       {IDS_OS_SETTINGS_TAG_AUDIO_SETTINGS_ALT1,
        IDS_OS_SETTINGS_TAG_AUDIO_SETTINGS_ALT2,
        IDS_OS_SETTINGS_TAG_AUDIO_SETTINGS_ALT3,
        IDS_OS_SETTINGS_TAG_AUDIO_SETTINGS_ALT4, SearchConcept::kAltTagEnd}},
  });
  return *tags;
}

const std::vector<SearchConcept>& GetKeyboardSearchConcepts() {
  static const base::NoDestructor<std::vector<SearchConcept>> tags(
      {{IDS_OS_SETTINGS_TAG_KEYBOARD,
        mojom::kKeyboardSubpagePath,
        mojom::SearchResultIcon::kKeyboard,
        mojom::SearchResultDefaultRank::kMedium,
        mojom::SearchResultType::kSubpage,
        {.subpage = mojom::Subpage::kKeyboard}},
       {IDS_OS_SETTINGS_TAG_KEYBOARD_AUTO_REPEAT,
        mojom::kKeyboardSubpagePath,
        mojom::SearchResultIcon::kKeyboard,
        mojom::SearchResultDefaultRank::kMedium,
        mojom::SearchResultType::kSetting,
        {.setting = mojom::Setting::kKeyboardAutoRepeat},
        {IDS_OS_SETTINGS_TAG_KEYBOARD_AUTO_REPEAT_ALT1,
         SearchConcept::kAltTagEnd}},
       {IDS_OS_SETTINGS_TAG_KEYBOARD_SHORTCUTS,
        mojom::kKeyboardSubpagePath,
        mojom::SearchResultIcon::kKeyboard,
        mojom::SearchResultDefaultRank::kMedium,
        mojom::SearchResultType::kSetting,
        {.setting = mojom::Setting::kKeyboardShortcuts}},
       {IDS_OS_SETTINGS_TAG_KEYBOARD_FUNCTION_KEYS,
        mojom::kKeyboardSubpagePath,
        mojom::SearchResultIcon::kKeyboard,
        mojom::SearchResultDefaultRank::kMedium,
        mojom::SearchResultType::kSetting,
        {.setting = mojom::Setting::kKeyboardFunctionKeys}},
       {IDS_OS_SETTINGS_TAG_KEYBOARD_DIACRITIC,
        mojom::kKeyboardSubpagePath,
        mojom::SearchResultIcon::kKeyboard,
        mojom::SearchResultDefaultRank::kMedium,
        mojom::SearchResultType::kSetting,
        {.setting = mojom::Setting::kShowDiacritic},
        {IDS_OS_SETTINGS_TAG_KEYBOARD_DIACRITIC1,
         IDS_OS_SETTINGS_TAG_KEYBOARD_DIACRITIC2,
         IDS_OS_SETTINGS_TAG_KEYBOARD_DIACRITIC3, SearchConcept::kAltTagEnd}}});
  return *tags;
}

const std::vector<SearchConcept>& GetPerDeviceKeyboardSearchConcepts() {
  static const base::NoDestructor<std::vector<SearchConcept>> tags({
      {IDS_OS_SETTINGS_TAG_KEYBOARD,
       mojom::kPerDeviceKeyboardSubpagePath,
       mojom::SearchResultIcon::kKeyboard,
       mojom::SearchResultDefaultRank::kMedium,
       mojom::SearchResultType::kSubpage,
       {.subpage = mojom::Subpage::kPerDeviceKeyboard}},
      {IDS_OS_SETTINGS_TAG_KEYBOARD_AUTO_REPEAT,
       mojom::kPerDeviceKeyboardSubpagePath,
       mojom::SearchResultIcon::kKeyboard,
       mojom::SearchResultDefaultRank::kMedium,
       mojom::SearchResultType::kSetting,
       {.setting = mojom::Setting::kKeyboardAutoRepeat},
       {IDS_OS_SETTINGS_TAG_KEYBOARD_AUTO_REPEAT_ALT1,
        SearchConcept::kAltTagEnd}},
      {IDS_OS_SETTINGS_TAG_KEYBOARD_SHORTCUTS,
       mojom::kPerDeviceKeyboardSubpagePath,
       mojom::SearchResultIcon::kKeyboard,
       mojom::SearchResultDefaultRank::kMedium,
       mojom::SearchResultType::kSetting,
       {.setting = mojom::Setting::kKeyboardShortcuts}},
      {IDS_OS_SETTINGS_TAG_KEYBOARD_FUNCTION_KEYS,
       mojom::kPerDeviceKeyboardSubpagePath,
       mojom::SearchResultIcon::kKeyboard,
       mojom::SearchResultDefaultRank::kMedium,
       mojom::SearchResultType::kSetting,
       {.setting = mojom::Setting::kKeyboardFunctionKeys}},
      {IDS_OS_SETTINGS_TAG_KEYBOARD_BLOCK_META_FKEY_COMBO_REWRITES,
       mojom::kPerDeviceKeyboardSubpagePath,
       mojom::SearchResultIcon::kKeyboard,
       mojom::SearchResultDefaultRank::kMedium,
       mojom::SearchResultType::kSetting,
       {.setting = mojom::Setting::kKeyboardBlockMetaFkeyRewrites}},
      {IDS_OS_SETTINGS_TAG_KEYBOARD_REMAP_KEYS,
       mojom::kPerDeviceKeyboardSubpagePath,
       mojom::SearchResultIcon::kKeyboard,
       mojom::SearchResultDefaultRank::kMedium,
       mojom::SearchResultType::kSetting,
       {.setting = mojom::Setting::kKeyboardRemapKeys}},
  });
  return *tags;
}

const std::vector<SearchConcept>& GetTouchpadSearchConcepts() {
  static const base::NoDestructor<std::vector<SearchConcept>> tags({
      {IDS_OS_SETTINGS_TAG_TOUCHPAD_SPEED,
       mojom::kPointersSubpagePath,
       mojom::SearchResultIcon::kLaptop,
       mojom::SearchResultDefaultRank::kMedium,
       mojom::SearchResultType::kSetting,
       {.setting = mojom::Setting::kTouchpadSpeed}},
      {IDS_OS_SETTINGS_TAG_TOUCHPAD_TAP_DRAGGING,
       mojom::kPointersSubpagePath,
       mojom::SearchResultIcon::kLaptop,
       mojom::SearchResultDefaultRank::kMedium,
       mojom::SearchResultType::kSetting,
       {.setting = mojom::Setting::kTouchpadTapDragging}},
      {IDS_OS_SETTINGS_TAG_TOUCHPAD_TAP_TO_CLICK,
       mojom::kPointersSubpagePath,
       mojom::SearchResultIcon::kLaptop,
       mojom::SearchResultDefaultRank::kMedium,
       mojom::SearchResultType::kSetting,
       {.setting = mojom::Setting::kTouchpadTapToClick}},
      {IDS_OS_SETTINGS_TAG_TOUCHPAD,
       mojom::kPointersSubpagePath,
       mojom::SearchResultIcon::kLaptop,
       mojom::SearchResultDefaultRank::kMedium,
       mojom::SearchResultType::kSubpage,
       {.subpage = mojom::Subpage::kPointers},
       {IDS_OS_SETTINGS_TAG_TOUCHPAD_ALT1, SearchConcept::kAltTagEnd}},
      {IDS_OS_SETTINGS_TAG_TOUCHPAD_REVERSE_SCROLLING,
       mojom::kPointersSubpagePath,
       mojom::SearchResultIcon::kLaptop,
       mojom::SearchResultDefaultRank::kMedium,
       mojom::SearchResultType::kSetting,
       {.setting = mojom::Setting::kTouchpadReverseScrolling}},
      {IDS_OS_SETTINGS_TAG_TOUCHPAD_ACCELERATION,
       mojom::kPointersSubpagePath,
       mojom::SearchResultIcon::kLaptop,
       mojom::SearchResultDefaultRank::kMedium,
       mojom::SearchResultType::kSetting,
       {.setting = mojom::Setting::kTouchpadAcceleration}},
  });
  return *tags;
}

const std::vector<SearchConcept>& GetPerDeviceTouchpadSearchConcepts() {
  static const base::NoDestructor<std::vector<SearchConcept>> tags({
      {IDS_OS_SETTINGS_TAG_TOUCHPAD_SPEED,
       mojom::kPerDeviceTouchpadSubpagePath,
       mojom::SearchResultIcon::kLaptop,
       mojom::SearchResultDefaultRank::kMedium,
       mojom::SearchResultType::kSetting,
       {.setting = mojom::Setting::kTouchpadSpeed}},
      {IDS_OS_SETTINGS_TAG_TOUCHPAD_TAP_DRAGGING,
       mojom::kPerDeviceTouchpadSubpagePath,
       mojom::SearchResultIcon::kLaptop,
       mojom::SearchResultDefaultRank::kMedium,
       mojom::SearchResultType::kSetting,
       {.setting = mojom::Setting::kTouchpadTapDragging}},
      {IDS_OS_SETTINGS_TAG_TOUCHPAD_TAP_TO_CLICK,
       mojom::kPerDeviceTouchpadSubpagePath,
       mojom::SearchResultIcon::kLaptop,
       mojom::SearchResultDefaultRank::kMedium,
       mojom::SearchResultType::kSetting,
       {.setting = mojom::Setting::kTouchpadTapToClick}},
      {IDS_OS_SETTINGS_TAG_TOUCHPAD,
       mojom::kPerDeviceTouchpadSubpagePath,
       mojom::SearchResultIcon::kLaptop,
       mojom::SearchResultDefaultRank::kMedium,
       mojom::SearchResultType::kSubpage,
       {.subpage = mojom::Subpage::kPerDeviceTouchpad},
       {IDS_OS_SETTINGS_TAG_TOUCHPAD_ALT1, SearchConcept::kAltTagEnd}},
      {IDS_OS_SETTINGS_TAG_TOUCHPAD_REVERSE_SCROLLING,
       mojom::kPerDeviceTouchpadSubpagePath,
       mojom::SearchResultIcon::kLaptop,
       mojom::SearchResultDefaultRank::kMedium,
       mojom::SearchResultType::kSetting,
       {.setting = mojom::Setting::kTouchpadReverseScrolling}},
      {IDS_OS_SETTINGS_TAG_TOUCHPAD_ACCELERATION,
       mojom::kPerDeviceTouchpadSubpagePath,
       mojom::SearchResultIcon::kLaptop,
       mojom::SearchResultDefaultRank::kMedium,
       mojom::SearchResultType::kSetting,
       {.setting = mojom::Setting::kTouchpadAcceleration}},
  });
  return *tags;
}

const std::vector<SearchConcept>& GetTouchpadHapticSearchConcepts() {
  static const base::NoDestructor<std::vector<SearchConcept>> tags({
      {IDS_OS_SETTINGS_TAG_TOUCHPAD_HAPTIC_FEEDBACK,
       mojom::kPointersSubpagePath,
       mojom::SearchResultIcon::kLaptop,
       mojom::SearchResultDefaultRank::kMedium,
       mojom::SearchResultType::kSetting,
       {.setting = mojom::Setting::kTouchpadHapticFeedback}},
      {IDS_OS_SETTINGS_TAG_TOUCHPAD_HAPTIC_CLICK_SENSITIVITY,
       mojom::kPointersSubpagePath,
       mojom::SearchResultIcon::kLaptop,
       mojom::SearchResultDefaultRank::kMedium,
       mojom::SearchResultType::kSetting,
       {.setting = mojom::Setting::kTouchpadHapticClickSensitivity}},
  });
  return *tags;
}

const std::vector<SearchConcept>& GetPerDeviceTouchpadHapticSearchConcepts() {
  static const base::NoDestructor<std::vector<SearchConcept>> tags({
      {IDS_OS_SETTINGS_TAG_TOUCHPAD_HAPTIC_FEEDBACK,
       mojom::kPerDeviceTouchpadSubpagePath,
       mojom::SearchResultIcon::kLaptop,
       mojom::SearchResultDefaultRank::kMedium,
       mojom::SearchResultType::kSetting,
       {.setting = mojom::Setting::kTouchpadHapticFeedback}},
      {IDS_OS_SETTINGS_TAG_TOUCHPAD_HAPTIC_CLICK_SENSITIVITY,
       mojom::kPerDeviceTouchpadSubpagePath,
       mojom::SearchResultIcon::kLaptop,
       mojom::SearchResultDefaultRank::kMedium,
       mojom::SearchResultType::kSetting,
       {.setting = mojom::Setting::kTouchpadHapticClickSensitivity}},
  });
  return *tags;
}

const std::vector<SearchConcept>& GetMouseScrollAccelerationSearchConcepts() {
  static const base::NoDestructor<std::vector<SearchConcept>> tags({
      {IDS_OS_SETTINGS_TAG_MOUSE_SCROLL_ACCELERATION,
       mojom::kPointersSubpagePath,
       mojom::SearchResultIcon::kMouse,
       mojom::SearchResultDefaultRank::kMedium,
       mojom::SearchResultType::kSetting,
       {.setting = mojom::Setting::kMouseScrollAcceleration}},
  });
  return *tags;
}

const std::vector<SearchConcept>&
GetPerDeviceMouseScrollAccelerationSearchConcepts() {
  static const base::NoDestructor<std::vector<SearchConcept>> tags({
      {IDS_OS_SETTINGS_TAG_MOUSE_SCROLL_ACCELERATION,
       mojom::kPerDeviceMouseSubpagePath,
       mojom::SearchResultIcon::kMouse,
       mojom::SearchResultDefaultRank::kMedium,
       mojom::SearchResultType::kSetting,
       {.setting = mojom::Setting::kMouseScrollAcceleration}},
  });
  return *tags;
}

const std::vector<SearchConcept>& GetMouseSearchConcepts() {
  static const base::NoDestructor<std::vector<SearchConcept>> tags({
      {IDS_OS_SETTINGS_TAG_MOUSE_ACCELERATION,
       mojom::kPointersSubpagePath,
       mojom::SearchResultIcon::kMouse,
       mojom::SearchResultDefaultRank::kMedium,
       mojom::SearchResultType::kSetting,
       {.setting = mojom::Setting::kMouseAcceleration}},
      {IDS_OS_SETTINGS_TAG_MOUSE_SWAP_BUTTON,
       mojom::kPointersSubpagePath,
       mojom::SearchResultIcon::kMouse,
       mojom::SearchResultDefaultRank::kMedium,
       mojom::SearchResultType::kSetting,
       {.setting = mojom::Setting::kMouseSwapPrimaryButtons}},
      {IDS_OS_SETTINGS_TAG_MOUSE_SPEED,
       mojom::kPointersSubpagePath,
       mojom::SearchResultIcon::kMouse,
       mojom::SearchResultDefaultRank::kMedium,
       mojom::SearchResultType::kSetting,
       {.setting = mojom::Setting::kMouseSpeed}},
      {IDS_OS_SETTINGS_TAG_MOUSE_REVERSE_SCROLLING,
       mojom::kPointersSubpagePath,
       mojom::SearchResultIcon::kMouse,
       mojom::SearchResultDefaultRank::kMedium,
       mojom::SearchResultType::kSetting,
       {.setting = mojom::Setting::kMouseReverseScrolling}},
      {IDS_OS_SETTINGS_TAG_MOUSE,
       mojom::kPointersSubpagePath,
       mojom::SearchResultIcon::kMouse,
       mojom::SearchResultDefaultRank::kMedium,
       mojom::SearchResultType::kSubpage,
       {.subpage = mojom::Subpage::kPointers}},
  });
  return *tags;
}

const std::vector<SearchConcept>& GetPerDeviceMouseSearchConcepts() {
  static const base::NoDestructor<std::vector<SearchConcept>> tags({
      {IDS_OS_SETTINGS_TAG_MOUSE_ACCELERATION,
       mojom::kPerDeviceMouseSubpagePath,
       mojom::SearchResultIcon::kMouse,
       mojom::SearchResultDefaultRank::kMedium,
       mojom::SearchResultType::kSetting,
       {.setting = mojom::Setting::kMouseAcceleration}},
      {IDS_OS_SETTINGS_TAG_MOUSE_SWAP_BUTTON,
       mojom::kPerDeviceMouseSubpagePath,
       mojom::SearchResultIcon::kMouse,
       mojom::SearchResultDefaultRank::kMedium,
       mojom::SearchResultType::kSetting,
       {.setting = mojom::Setting::kMouseSwapPrimaryButtons}},
      {IDS_OS_SETTINGS_TAG_MOUSE_SPEED,
       mojom::kPerDeviceMouseSubpagePath,
       mojom::SearchResultIcon::kMouse,
       mojom::SearchResultDefaultRank::kMedium,
       mojom::SearchResultType::kSetting,
       {.setting = mojom::Setting::kMouseSpeed}},
      {IDS_OS_SETTINGS_TAG_MOUSE_REVERSE_SCROLLING,
       mojom::kPerDeviceMouseSubpagePath,
       mojom::SearchResultIcon::kMouse,
       mojom::SearchResultDefaultRank::kMedium,
       mojom::SearchResultType::kSetting,
       {.setting = mojom::Setting::kMouseReverseScrolling}},
      {IDS_OS_SETTINGS_TAG_MOUSE,
       mojom::kPerDeviceMouseSubpagePath,
       mojom::SearchResultIcon::kMouse,
       mojom::SearchResultDefaultRank::kMedium,
       mojom::SearchResultType::kSubpage,
       {.subpage = mojom::Subpage::kPerDeviceMouse}},
  });
  return *tags;
}

const std::vector<SearchConcept>& GetPointingStickSearchConcepts() {
  static const base::NoDestructor<std::vector<SearchConcept>> tags({
      {IDS_OS_SETTINGS_TAG_POINTING_STICK_PRIMARY_BUTTON,
       mojom::kPointersSubpagePath,
       mojom::SearchResultIcon::kLaptop,
       mojom::SearchResultDefaultRank::kMedium,
       mojom::SearchResultType::kSetting,
       {.setting = mojom::Setting::kPointingStickSwapPrimaryButtons}},
      {IDS_OS_SETTINGS_TAG_POINTING_STICK_ACCELERATION,
       mojom::kPointersSubpagePath,
       mojom::SearchResultIcon::kLaptop,
       mojom::SearchResultDefaultRank::kMedium,
       mojom::SearchResultType::kSetting,
       {.setting = mojom::Setting::kPointingStickAcceleration}},
      {IDS_OS_SETTINGS_TAG_POINTING_STICK_SPEED,
       mojom::kPointersSubpagePath,
       mojom::SearchResultIcon::kLaptop,
       mojom::SearchResultDefaultRank::kMedium,
       mojom::SearchResultType::kSetting,
       {.setting = mojom::Setting::kPointingStickSpeed}},
  });
  return *tags;
}

const std::vector<SearchConcept>& GetPerDevicePointingStickSearchConcepts() {
  static const base::NoDestructor<std::vector<SearchConcept>> tags({
      {IDS_OS_SETTINGS_TAG_POINTING_STICK_PRIMARY_BUTTON,
       mojom::kPerDevicePointingStickSubpagePath,
       mojom::SearchResultIcon::kLaptop,
       mojom::SearchResultDefaultRank::kMedium,
       mojom::SearchResultType::kSetting,
       {.setting = mojom::Setting::kPointingStickSwapPrimaryButtons}},
      {IDS_OS_SETTINGS_TAG_POINTING_STICK_ACCELERATION,
       mojom::kPerDevicePointingStickSubpagePath,
       mojom::SearchResultIcon::kLaptop,
       mojom::SearchResultDefaultRank::kMedium,
       mojom::SearchResultType::kSetting,
       {.setting = mojom::Setting::kPointingStickAcceleration}},
      {IDS_OS_SETTINGS_TAG_POINTING_STICK_SPEED,
       mojom::kPerDevicePointingStickSubpagePath,
       mojom::SearchResultIcon::kLaptop,
       mojom::SearchResultDefaultRank::kMedium,
       mojom::SearchResultType::kSetting,
       {.setting = mojom::Setting::kPointingStickSpeed}},
  });
  return *tags;
}

const std::vector<SearchConcept>& GetStylusSearchConcepts() {
  static const base::NoDestructor<std::vector<SearchConcept>> tags({
      {IDS_OS_SETTINGS_TAG_STYLUS_NOTE_APP,
       mojom::kStylusSubpagePath,
       mojom::SearchResultIcon::kStylus,
       mojom::SearchResultDefaultRank::kMedium,
       mojom::SearchResultType::kSetting,
       {.setting = mojom::Setting::kStylusNoteTakingApp},
       {IDS_OS_SETTINGS_TAG_STYLUS_NOTE_APP_ALT1,
        IDS_OS_SETTINGS_TAG_STYLUS_NOTE_APP_ALT2, SearchConcept::kAltTagEnd}},
      {IDS_OS_SETTINGS_TAG_STYLUS_LOCK_SCREEN_LATEST_NOTE,
       mojom::kStylusSubpagePath,
       mojom::SearchResultIcon::kStylus,
       mojom::SearchResultDefaultRank::kMedium,
       mojom::SearchResultType::kSetting,
       {.setting = mojom::Setting::kStylusLatestNoteOnLockScreen}},
      {IDS_OS_SETTINGS_TAG_STYLUS_LOCK_SCREEN_NOTES,
       mojom::kStylusSubpagePath,
       mojom::SearchResultIcon::kStylus,
       mojom::SearchResultDefaultRank::kMedium,
       mojom::SearchResultType::kSetting,
       {.setting = mojom::Setting::kStylusNoteTakingFromLockScreen}},
      {IDS_OS_SETTINGS_TAG_STYLUS_SHELF_TOOLS,
       mojom::kStylusSubpagePath,
       mojom::SearchResultIcon::kStylus,
       mojom::SearchResultDefaultRank::kMedium,
       mojom::SearchResultType::kSetting,
       {.setting = mojom::Setting::kStylusToolsInShelf},
       {IDS_OS_SETTINGS_TAG_STYLUS_SHELF_TOOLS_ALT1,
        IDS_OS_SETTINGS_TAG_STYLUS_SHELF_TOOLS_ALT2,
        SearchConcept::kAltTagEnd}},
      {IDS_OS_SETTINGS_TAG_STYLUS,
       mojom::kStylusSubpagePath,
       mojom::SearchResultIcon::kStylus,
       mojom::SearchResultDefaultRank::kMedium,
       mojom::SearchResultType::kSubpage,
       {.subpage = mojom::Subpage::kStylus}},
  });
  return *tags;
}

const std::vector<SearchConcept>& GetAudioPowerSoundsSearchConcepts() {
  static const base::NoDestructor<std::vector<SearchConcept>> tags({
      {
          IDS_OS_SETTINGS_TAG_CHARGING_SOUNDS,
          mojom::kAudioSubpagePath,
          mojom::SearchResultIcon::kAudio,
          mojom::SearchResultDefaultRank::kMedium,
          mojom::SearchResultType::kSetting,
          {.setting = mojom::Setting::kChargingSounds},
      },
      {
          IDS_OS_SETTINGS_TAG_LOW_BATTERY_SOUND,
          mojom::kAudioSubpagePath,
          mojom::SearchResultIcon::kAudio,
          mojom::SearchResultDefaultRank::kMedium,
          mojom::SearchResultType::kSetting,
          {.setting = mojom::Setting::kLowBatterySound},
      },
  });
  return *tags;
}

const std::vector<SearchConcept>& GetDisplayArrangementSearchConcepts() {
  static const base::NoDestructor<std::vector<SearchConcept>> tags({
      {IDS_OS_SETTINGS_TAG_DISPLAY_ARRANGEMENT,
       mojom::kDisplaySubpagePath,
       mojom::SearchResultIcon::kDisplay,
       mojom::SearchResultDefaultRank::kMedium,
       mojom::SearchResultType::kSetting,
       {.setting = mojom::Setting::kDisplayArrangement},
       {IDS_OS_SETTINGS_TAG_DISPLAY_ARRANGEMENT_ALT1,
        IDS_OS_SETTINGS_TAG_DISPLAY_ARRANGEMENT_ALT2,
        SearchConcept::kAltTagEnd}},
  });
  return *tags;
}

const std::vector<SearchConcept>& GetDisplayMirrorSearchConcepts() {
  static const base::NoDestructor<std::vector<SearchConcept>> tags({
      {IDS_OS_SETTINGS_TAG_MIRRORING,
       mojom::kDisplaySubpagePath,
       mojom::SearchResultIcon::kDisplay,
       mojom::SearchResultDefaultRank::kMedium,
       mojom::SearchResultType::kSetting,
       {.setting = mojom::Setting::kDisplayMirroring}},
  });
  return *tags;
}

const std::vector<SearchConcept>& GetDisplayUnifiedDesktopSearchConcepts() {
  static const base::NoDestructor<std::vector<SearchConcept>> tags({
      {IDS_OS_SETTINGS_TAG_UNIFIED_DESKTOP,
       mojom::kDisplaySubpagePath,
       mojom::SearchResultIcon::kDisplay,
       mojom::SearchResultDefaultRank::kMedium,
       mojom::SearchResultType::kSetting,
       {.setting = mojom::Setting::kAllowWindowsToSpanDisplays}},
  });
  return *tags;
}

const std::vector<SearchConcept>& GetDisplayExternalSearchConcepts() {
  static const base::NoDestructor<std::vector<SearchConcept>> tags({
      {IDS_OS_SETTINGS_TAG_DISPLAY_RESOLUTION,
       mojom::kDisplaySubpagePath,
       mojom::SearchResultIcon::kDisplay,
       mojom::SearchResultDefaultRank::kMedium,
       mojom::SearchResultType::kSetting,
       {.setting = mojom::Setting::kDisplayResolution},
       {IDS_OS_SETTINGS_TAG_DISPLAY_RESOLUTION_ALT1,
        IDS_OS_SETTINGS_TAG_DISPLAY_RESOLUTION_ALT2,
        SearchConcept::kAltTagEnd}},
      {IDS_OS_SETTINGS_TAG_DISPLAY_OVERSCAN,
       mojom::kDisplaySubpagePath,
       mojom::SearchResultIcon::kDisplay,
       mojom::SearchResultDefaultRank::kMedium,
       mojom::SearchResultType::kSetting,
       {.setting = mojom::Setting::kDisplayOverscan}},
  });
  return *tags;
}

const std::vector<SearchConcept>&
GetDisplayExternalWithRefreshSearchConcepts() {
  static const base::NoDestructor<std::vector<SearchConcept>> tags({
      {IDS_OS_SETTINGS_TAG_DISPLAY_REFRESH_RATE,
       mojom::kDisplaySubpagePath,
       mojom::SearchResultIcon::kDisplay,
       mojom::SearchResultDefaultRank::kMedium,
       mojom::SearchResultType::kSetting,
       {.setting = mojom::Setting::kDisplayRefreshRate},
       {IDS_OS_SETTINGS_TAG_DISPLAY_REFRESH_RATE_ALT1,
        IDS_OS_SETTINGS_TAG_DISPLAY_REFRESH_RATE_ALT2,
        SearchConcept::kAltTagEnd}},
  });
  return *tags;
}

const std::vector<SearchConcept>& GetDisplayOrientationSearchConcepts() {
  static const base::NoDestructor<std::vector<SearchConcept>> tags({
      {IDS_OS_SETTINGS_TAG_DISPLAY_ORIENTATION,
       mojom::kDisplaySubpagePath,
       mojom::SearchResultIcon::kDisplay,
       mojom::SearchResultDefaultRank::kMedium,
       mojom::SearchResultType::kSetting,
       {.setting = mojom::Setting::kDisplayOrientation},
       {IDS_OS_SETTINGS_TAG_DISPLAY_ORIENTATION_ALT1,
        SearchConcept::kAltTagEnd}},
  });
  return *tags;
}

const std::vector<SearchConcept>& GetDisplayAmbientSearchConcepts() {
  static const base::NoDestructor<std::vector<SearchConcept>> tags({
      {IDS_OS_SETTINGS_TAG_DISPLAY_AMBIENT_COLORS,
       mojom::kDisplaySubpagePath,
       mojom::SearchResultIcon::kDisplay,
       mojom::SearchResultDefaultRank::kMedium,
       mojom::SearchResultType::kSetting,
       {.setting = mojom::Setting::kAmbientColors}},
  });
  return *tags;
}

const std::vector<SearchConcept>& GetDisplayTouchCalibrationSearchConcepts() {
  static const base::NoDestructor<std::vector<SearchConcept>> tags({
      {IDS_OS_SETTINGS_TAG_DISPLAY_TOUCHSCREEN_CALIBRATION,
       mojom::kDisplaySubpagePath,
       mojom::SearchResultIcon::kDisplay,
       mojom::SearchResultDefaultRank::kMedium,
       mojom::SearchResultType::kSetting,
       {.setting = mojom::Setting::kTouchscreenCalibration}},
  });
  return *tags;
}

const std::vector<SearchConcept>& GetDisplayNightLightOnSearchConcepts() {
  static const base::NoDestructor<std::vector<SearchConcept>> tags({
      {IDS_OS_SETTINGS_TAG_NIGHT_LIGHT_COLOR_TEMPERATURE,
       mojom::kDisplaySubpagePath,
       mojom::SearchResultIcon::kDisplay,
       mojom::SearchResultDefaultRank::kLow,
       mojom::SearchResultType::kSetting,
       {.setting = mojom::Setting::kNightLightColorTemperature}},
  });
  return *tags;
}

const std::vector<SearchConcept>& GetExternalStorageSearchConcepts() {
  static const base::NoDestructor<std::vector<SearchConcept>> tags({
      {IDS_OS_SETTINGS_TAG_EXTERNAL_STORAGE,
       mojom::kExternalStorageSubpagePath,
       mojom::SearchResultIcon::kHardDrive,
       mojom::SearchResultDefaultRank::kMedium,
       mojom::SearchResultType::kSubpage,
       {.subpage = mojom::Subpage::kExternalStorage}},
  });
  return *tags;
}

const std::vector<SearchConcept>& GetPowerWithBatterySearchConcepts() {
  static const base::NoDestructor<std::vector<SearchConcept>> tags({
      {IDS_OS_SETTINGS_TAG_POWER_SOURCE,
       mojom::kPowerSubpagePath,
       mojom::SearchResultIcon::kPower,
       mojom::SearchResultDefaultRank::kMedium,
       mojom::SearchResultType::kSetting,
       {.setting = mojom::Setting::kPowerSource},
       {IDS_OS_SETTINGS_TAG_POWER_SOURCE_ALT1,
        IDS_OS_SETTINGS_TAG_POWER_SOURCE_ALT2, SearchConcept::kAltTagEnd}},
  });
  return *tags;
}

const std::vector<SearchConcept>& GetPowerWithLaptopLidSearchConcepts() {
  static const base::NoDestructor<std::vector<SearchConcept>> tags({
      {IDS_OS_SETTINGS_TAG_POWER_SLEEP_COVER_CLOSED,
       mojom::kPowerSubpagePath,
       mojom::SearchResultIcon::kPower,
       mojom::SearchResultDefaultRank::kMedium,
       mojom::SearchResultType::kSetting,
       {.setting = mojom::Setting::kSleepWhenLaptopLidClosed},
       {IDS_OS_SETTINGS_TAG_POWER_SLEEP_COVER_CLOSED_ALT1,
        IDS_OS_SETTINGS_TAG_POWER_SLEEP_COVER_CLOSED_ALT2,
        SearchConcept::kAltTagEnd}},
  });
  return *tags;
}

const std::vector<SearchConcept>& GetPowerWithAdaptiveChargingSearchConcepts() {
  static const base::NoDestructor<std::vector<SearchConcept>> tags({
      {IDS_OS_SETTINGS_TAG_POWER_ADAPTIVE_CHARGING,
       mojom::kPowerSubpagePath,
       mojom::SearchResultIcon::kPower,
       mojom::SearchResultDefaultRank::kMedium,
       mojom::SearchResultType::kSetting,
       {.setting = mojom::Setting::kAdaptiveCharging},
       {}},
  });
  return *tags;
}

const std::vector<SearchConcept>& GetPowerWithBatterySaverModeSearchConcepts() {
  static const base::NoDestructor<std::vector<SearchConcept>> tags({
      {IDS_OS_SETTINGS_TAG_POWER_BATTERY_SAVER,
       mojom::kPowerSubpagePath,
       mojom::SearchResultIcon::kPower,
       mojom::SearchResultDefaultRank::kMedium,
       mojom::SearchResultType::kSetting,
       {.setting = mojom::Setting::kBatterySaver},
       {}},
  });
  return *tags;
}

bool IsUnifiedDesktopAvailable() {
  return base::CommandLine::ForCurrentProcess()->HasSwitch(
      ::switches::kEnableUnifiedDesktop);
}

bool DoesDeviceSupportAmbientColor() {
  return ash::features::IsAllowAmbientEQEnabled();
}

bool IsTouchCalibrationAvailable() {
  return base::CommandLine::ForCurrentProcess()->HasSwitch(
             switches::kEnableTouchCalibrationSetting) &&
         display::HasExternalTouchscreenDevice();
}

bool IsListAllDisplayModesEnabled() {
  return display::features::IsListAllDisplayModesEnabled();
}

bool IsShowForceRespectUiGainsToggleEnabled() {
  // No need to show the toggle if UI gains is not going to be ignored.
  if (!base::FeatureList::IsEnabled(media::kIgnoreUiGains)) {
    return false;
  }
  return base::FeatureList::IsEnabled(media::kShowForceRespectUiGainsToggle);
}

void AddDeviceKeyboardStrings(content::WebUIDataSource* html_source) {
  const bool kIsRevampEnabled =
      ash::features::IsOsSettingsRevampWayfindingEnabled();

  webui::LocalizedString keyboard_strings[] = {
      {"builtInKeyboardName", IDS_SETTINGS_BUILT_IN_KEYBOARD_NAME},
      {"keyboardEnableAutoRepeat", IDS_SETTINGS_KEYBOARD_AUTO_REPEAT_ENABLE},
      {"keyboardEnableAutoRepeatSubLabel",
       IDS_SETTINGS_KEYBOARD_AUTO_REPEAT_ENABLE_SUB_LABEL},
      {"keyboardKeyAlt", IDS_SETTINGS_KEYBOARD_KEY_LEFT_ALT},
      {"keyboardKeyAssistant", IDS_SETTINGS_KEYBOARD_KEY_ASSISTANT},
      {"keyboardKeyBackspace", IDS_SETTINGS_KEYBOARD_KEY_BACKSPACE},
      {"keyboardKeyCapsLock", IDS_SETTINGS_KEYBOARD_KEY_CAPS_LOCK},
      {"keyboardKeyCommand", IDS_SETTINGS_KEYBOARD_KEY_COMMAND},
      {"keyboardKeyCtrl", IDS_SETTINGS_KEYBOARD_KEY_LEFT_CTRL},
      {"keyboardKeyDiamond", IDS_SETTINGS_KEYBOARD_KEY_DIAMOND},
      {"keyboardKeyDisabled", IDS_SETTINGS_KEYBOARD_KEY_DISABLED},
      {"keyboardKeyEscape", IDS_SETTINGS_KEYBOARD_KEY_ESCAPE},
      {"keyboardKeyExternalCommand",
       IDS_SETTINGS_KEYBOARD_KEY_EXTERNAL_COMMAND},
      {"keyboardKeyExternalMeta", IDS_SETTINGS_KEYBOARD_KEY_EXTERNAL_META},
      {"keyboardKeyMeta", IDS_SETTINGS_KEYBOARD_KEY_META},
      {"keyboardSendFunctionKeys", IDS_SETTINGS_KEYBOARD_SEND_FUNCTION_KEYS},
      {"keyboardSendInvertedFunctionKeys",
       IDS_SETTINGS_KEYBOARD_SEND_INVERTED_FUNCTION_KEYS},
      {"keyboardSendInvertedFunctionKeysDescription",
       IDS_SETTINGS_KEYBOARD_SEND_INVERTED_FUNCTION_KEYS_DESCRIPTION},
      {"keyboardShowInputSettings",
       kIsRevampEnabled ? IDS_OS_SETTINGS_REVAMP_KEYBOARD_SHOW_INPUT_SETTINGS
                        : IDS_SETTINGS_KEYBOARD_SHOW_INPUT_SETTINGS},
      // TODO(crbug.com/1097328): Remove this string, as it is unused.
      {"keyboardShowLanguageAndInput",
       IDS_SETTINGS_KEYBOARD_SHOW_LANGUAGE_AND_INPUT},
      {"keyboardTitle", IDS_SETTINGS_KEYBOARD_TITLE},
      {"keyRepeatDelay", IDS_SETTINGS_KEYBOARD_AUTO_REPEAT_DELAY},
      {"keyRepeatDelayLong", IDS_SETTINGS_KEYBOARD_AUTO_REPEAT_DELAY_LONG},
      {"keyRepeatDelayShort", IDS_SETTINGS_KEYBOARD_AUTO_REPEAT_DELAY_SHORT},
      {"keyRepeatRate", IDS_SETTINGS_KEYBOARD_AUTO_REPEAT_RATE},
      {"keyRepeatRateFast", IDS_SETTINGS_KEYBOARD_AUTO_REPEAT_FAST},
      {"keyRepeatRateSlow", IDS_SETTINGS_KEYBOARD_AUTO_REPEAT_RATE_SLOW},
      {"remapKeyboardKeysRowLabel", IDS_SETTINGS_KEYBOARD_REMAP_KEYS_ROW_LABEL},
      {"remapKeyboardKeysDescription",
       IDS_SETTINGS_KEYBOARD_REMAP_KEYS_DESCRIPTION},
      {"showKeyboardShortcutViewer",
       IDS_SETTINGS_KEYBOARD_SHOW_SHORTCUT_VIEWER},
      {"viewAndCustomizeKeyboardShortcut",
       IDS_SETTINGS_KEYBOARD_VIEW_AND_CUSTOMIZE_SHORTCUTS},
      {"keyboardKeyLauncher", IDS_SETTINGS_KEYBOARD_KEY_LAUNCHER},
      {"keyboardKeySearch", IDS_SETTINGS_KEYBOARD_KEY_SEARCH},
      {"keyboardRemapRestoreDefaultsLabel",
       IDS_SETTINGS_KEYBOARD_REMAP_RESTORE_BUTTON_LABEL},
      {"keyboardHoldingKeys", IDS_SETTINGS_KEYBOARD_HOLDING_KEYS},
      {"keyboardAccentMarks", IDS_SETTINGS_KEYBOARD_ACCENT_MARKS},
      {"keyboardAccentMarksSubLabel",
       IDS_SETTINGS_KEYBOARD_ACCENT_MARKS_SUB_LABEL},
      {"noKeyboardsConnected", IDS_SETTINGS_KEYBOARD_NO_KEYBOARDS_HELP_MESSAGE},
      {"perDeviceKeyboardKeyAlt",
       IDS_SETTINGS_PER_DEVICE_KEYBOARD_KEY_LEFT_ALT},
      {"perDeviceKeyboardKeyAssistant",
       IDS_SETTINGS_PER_DEVICE_KEYBOARD_KEY_ASSISTANT},
      {"perDeviceKeyboardKeyBackspace",
       IDS_SETTINGS_PER_DEVICE_KEYBOARD_KEY_BACKSPACE},
      {"perDeviceKeyboardKeyCapsLock",
       IDS_SETTINGS_PER_DEVICE_KEYBOARD_KEY_CAPS_LOCK},
      {"perDeviceKeyboardKeyCommand",
       IDS_SETTINGS_PER_DEVICE_KEYBOARD_KEY_COMMAND},
      {"perDeviceKeyboardKeyCtrl",
       IDS_SETTINGS_PER_DEVICE_KEYBOARD_KEY_LEFT_CTRL},
      {"perDeviceKeyboardKeyDisabled",
       IDS_SETTINGS_PER_DEVICE_KEYBOARD_KEY_DISABLED},
      {"perDeviceKeyboardKeyEscape",
       IDS_SETTINGS_PER_DEVICE_KEYBOARD_KEY_ESCAPE},
      {"perDeviceKeyboardKeyMeta", IDS_SETTINGS_PER_DEVICE_KEYBOARD_KEY_META},
  };
  html_source->AddLocalizedStrings(keyboard_strings);

  if (Shell::Get()->keyboard_capability()->HasLauncherButtonOnAnyKeyboard()) {
    html_source->AddLocalizedString(
        "keyboardBlockMetaFunctionKeyRewrites",
        IDS_SETTINGS_KEYBOARD_BLOCK_META_FUNCTION_KEY_REWRITES_LAUNCHER);
    html_source->AddLocalizedString(
        "keyboardBlockMetaFunctionKeyRewritesDescription",
        IDS_SETTINGS_KEYBOARD_BLOCK_META_FUNCTION_KEY_REWRITES_DESCRIPTION_LAUNCHER);
    html_source->AddLocalizedString(
        "perDeviceKeyboardKeySearch",
        IDS_SETTINGS_PER_DEVICE_KEYBOARD_KEY_LAUNCHER);
    html_source->AddLocalizedString("keyboardKeySearch",
                                    IDS_SETTINGS_KEYBOARD_KEY_LAUNCHER);
    html_source->AddLocalizedString(
        "keyboardSendFunctionKeysDescription",
        IDS_SETTINGS_KEYBOARD_SEND_FUNCTION_KEYS_LAYOUT2_DESCRIPTION);
    html_source->AddLocalizedString("sixPackKeyDeleteSearch",
                                    IDS_SETTINGS_SIX_PACK_KEY_DELETE_LAUNCHER);
    html_source->AddLocalizedString("sixPackKeyHomeSearch",
                                    IDS_SETTINGS_SIX_PACK_KEY_HOME_LAUNCHER);
    html_source->AddLocalizedString("sixPackKeyEndSearch",
                                    IDS_SETTINGS_SIX_PACK_KEY_END_LAUNCHER);
    html_source->AddLocalizedString("sixPackKeyPageUpSearch",
                                    IDS_SETTINGS_SIX_PACK_KEY_PAGE_UP_LAUNCHER);
    html_source->AddLocalizedString(
        "sixPackKeyPageDownSearch",
        IDS_SETTINGS_SIX_PACK_KEY_PAGE_DOWN_LAUNCHER);
    html_source->AddLocalizedString("sixPackKeyInsertSearch",
                                    IDS_SETTINGS_SIX_PACK_KEY_INSERT_LAUNCHER);
    html_source->AddLocalizedString(
        "touchpadSimulateRightClickOptionSearch",
        IDS_SETTINGS_TOUCHPAD_SIMULATE_RIGHT_CLICK_OPTION_LAUNCHER);
  } else {
    html_source->AddLocalizedString(
        "keyboardBlockMetaFunctionKeyRewrites",
        IDS_SETTINGS_KEYBOARD_BLOCK_META_FUNCTION_KEY_REWRITES_SEARCH);
    html_source->AddLocalizedString(
        "keyboardBlockMetaFunctionKeyRewritesDescription",
        IDS_SETTINGS_KEYBOARD_BLOCK_META_FUNCTION_KEY_REWRITES_DESCRIPTION_SEARCH);
    html_source->AddLocalizedString(
        "perDeviceKeyboardKeySearch",
        IDS_SETTINGS_PER_DEVICE_KEYBOARD_KEY_SEARCH);
    html_source->AddLocalizedString("keyboardKeySearch",
                                    IDS_SETTINGS_KEYBOARD_KEY_SEARCH);
    html_source->AddLocalizedString(
        "keyboardSendFunctionKeysDescription",
        IDS_SETTINGS_KEYBOARD_SEND_FUNCTION_KEYS_DESCRIPTION);
    html_source->AddLocalizedString("sixPackKeyDeleteSearch",
                                    IDS_SETTINGS_SIX_PACK_KEY_DELETE_SEARCH);
    html_source->AddLocalizedString("sixPackKeyHomeSearch",
                                    IDS_SETTINGS_SIX_PACK_KEY_HOME_SEARCH);
    html_source->AddLocalizedString("sixPackKeyEndSearch",
                                    IDS_SETTINGS_SIX_PACK_KEY_END_SEARCH);
    html_source->AddLocalizedString("sixPackKeyPageUpSearch",
                                    IDS_SETTINGS_SIX_PACK_KEY_PAGE_UP_SEARCH);
    html_source->AddLocalizedString("sixPackKeyPageDownSearch",
                                    IDS_SETTINGS_SIX_PACK_KEY_PAGE_DOWN_SEARCH);
    html_source->AddLocalizedString("sixPackKeyInsertSearch",
                                    IDS_SETTINGS_SIX_PACK_KEY_INSERT_SEARCH);
    html_source->AddLocalizedString(
        "touchpadSimulateRightClickOptionSearch",
        IDS_SETTINGS_TOUCHPAD_SIMULATE_RIGHT_CLICK_OPTION_SEARCH);
  }
}

void AddDeviceStylusStrings(content::WebUIDataSource* html_source) {
  static constexpr webui::LocalizedString kStylusStrings[] = {
      {"stylusAutoOpenStylusTools", IDS_SETTINGS_STYLUS_AUTO_OPEN_STYLUS_TOOLS},
      {"stylusEnableStylusTools", IDS_SETTINGS_STYLUS_ENABLE_STYLUS_TOOLS},
      {"stylusFindMoreAppsPrimary", IDS_SETTINGS_STYLUS_FIND_MORE_APPS_PRIMARY},
      {"stylusFindMoreAppsSecondary",
       IDS_SETTINGS_STYLUS_FIND_MORE_APPS_SECONDARY},
      {"stylusNoteTakingApp", IDS_SETTINGS_STYLUS_NOTE_TAKING_APP_LABEL},
      {"stylusNoteTakingAppEnabledOnLockScreen",
       IDS_SETTINGS_STYLUS_NOTE_TAKING_APP_LOCK_SCREEN_CHECKBOX},
      {"stylusNoteTakingAppKeepsLastNoteOnLockScreen",
       IDS_SETTINGS_STYLUS_NOTE_TAKING_APP_KEEP_LATEST_NOTE},
      {"stylusNoteTakingAppLockScreenSettingsHeader",
       IDS_SETTINGS_STYLUS_LOCK_SCREEN_NOTES_TITLE},
      {"stylusNoteTakingAppNoneAvailable",
       IDS_SETTINGS_STYLUS_NOTE_TAKING_APP_NONE_AVAILABLE},
      {"stylusNoteTakingAppWaitingForAndroid",
       IDS_SETTINGS_STYLUS_NOTE_TAKING_APP_WAITING_FOR_ANDROID},
      {"stylusTitle", IDS_SETTINGS_STYLUS_TITLE}};
  html_source->AddLocalizedStrings(kStylusStrings);

  html_source->AddBoolean("hasInternalStylus",
                          stylus_utils::HasInternalStylus());
}

void AddDeviceStorageStrings(content::WebUIDataSource* html_source,
                             bool is_external_storage_page_available) {
  static constexpr webui::LocalizedString kStorageStrings[] = {
      {"storageExternal", IDS_SETTINGS_STORAGE_EXTERNAL},
      {"storageExternalStorageEmptyListHeader",
       IDS_SETTINGS_STORAGE_EXTERNAL_STORAGE_EMPTY_LIST_HEADER},
      {"storageExternalStorageListHeader",
       IDS_SETTINGS_STORAGE_EXTERNAL_STORAGE_LIST_HEADER},
      {"storageItemApps", IDS_SETTINGS_STORAGE_ITEM_APPS},
      {"storageItemOffline", IDS_SETTINGS_STORAGE_ITEM_OFFLINE},
      {"storageItemAvailable", IDS_SETTINGS_STORAGE_ITEM_AVAILABLE},
      {"storageItemBrowsingData", IDS_SETTINGS_STORAGE_ITEM_BROWSING_DATA},
      {"storageItemCrostini", IDS_SETTINGS_STORAGE_ITEM_CROSTINI},
      {"storageItemInUse", IDS_SETTINGS_STORAGE_ITEM_IN_USE},
      {"storageItemMyFiles", IDS_SETTINGS_STORAGE_ITEM_MY_FILES},
      {"storageItemOtherUsers", IDS_SETTINGS_STORAGE_ITEM_OTHER_USERS},
      {"storageItemSystem", IDS_SETTINGS_STORAGE_ITEM_SYSTEM},
      {"storageOverviewAriaLabel", IDS_SETTINGS_STORAGE_OVERVIEW_ARIA_LABEL},
      {"storageSizeComputing", IDS_SETTINGS_STORAGE_SIZE_CALCULATING},
      {"storageSizeUnknown", IDS_SETTINGS_STORAGE_SIZE_UNKNOWN},
      {"storageSpaceCriticallyLowMessageLine1",
       IDS_SETTINGS_STORAGE_SPACE_CRITICALLY_LOW_MESSAGE_LINE_1},
      {"storageSpaceCriticallyLowMessageLine2",
       IDS_SETTINGS_STORAGE_SPACE_CRITICALLY_LOW_MESSAGE_LINE_2},
      {"storageSpaceCriticallyLowMessageTitle",
       IDS_SETTINGS_STORAGE_SPACE_CRITICALLY_LOW_MESSAGE_TITLE},
      {"storageSpaceLowMessageLine1",
       IDS_SETTINGS_STORAGE_SPACE_LOW_MESSAGE_LINE_1},
      {"storageSpaceLowMessageLine2",
       IDS_SETTINGS_STORAGE_SPACE_LOW_MESSAGE_LINE_2},
      {"storageSpaceLowMessageTitle",
       IDS_SETTINGS_STORAGE_SPACE_LOW_MESSAGE_TITLE},
      {"storageTitle", IDS_SETTINGS_STORAGE_TITLE},
  };
  html_source->AddLocalizedStrings(kStorageStrings);

  html_source->AddBoolean("androidEnabled", is_external_storage_page_available);

  html_source->AddString(
      "storageAndroidAppsExternalDrivesNote",
      l10n_util::GetStringFUTF16(
          IDS_SETTINGS_STORAGE_ANDROID_APPS_ACCESS_EXTERNAL_DRIVES_NOTE,
          base::ASCIIToUTF16(chrome::kArcExternalStorageLearnMoreURL)));
}

void AddDeviceAudioStrings(content::WebUIDataSource* html_source) {
  static constexpr webui::LocalizedString kAudioStrings[] = {
      {"audioDeviceBluetoothLabel", IDS_SETTINGS_AUDIO_DEVICE_BLUETOOTH_LABEL},
      {"audioDeviceFrontMicLabel", IDS_SETTINGS_AUDIO_DEVICE_FRONT_MIC_LABEL},
      {"audioDeviceHdmiLabel", IDS_SETTINGS_AUDIO_DEVICE_HDMI_LABEL},
      {"audioDeviceHeadphoneLabel", IDS_SETTINGS_AUDIO_DEVICE_HEADPHONE_LABEL},
      {"audioDeviceInternalMicLabel",
       IDS_SETTINGS_AUDIO_DEVICE_INTERNAL_MIC_LABEL},
      {"audioDeviceInternalSpeakersLabel",
       IDS_SETTINGS_AUDIO_DEVICE_INTERNAL_SPEAKERS_LABEL},
      {"audioDeviceMicJackLabel", IDS_SETTINGS_AUDIO_DEVICE_MIC_JACK_LABEL},
      {"audioDeviceRearMicLabel", IDS_SETTINGS_AUDIO_DEVICE_REAR_MIC_LABEL},
      {"audioDeviceUsbLabel", IDS_SETTINGS_AUDIO_DEVICE_USB_LABEL},
      {"audioInputDeviceTitle", IDS_SETTINGS_AUDIO_INPUT_DEVICE_TITLE},
      {"audioInputAllowAGCTitle", IDS_SETTINGS_AUDIO_INPUT_ALLOW_AGC_TITLE},
      {"audioInputGainTitle", IDS_SETTINGS_AUDIO_INPUT_GAIN_TITLE},
      {"audioInputMuteButtonAriaLabelMuted",
       IDS_SETTINGS_AUDIO_INPUT_MUTE_BUTTON_ARIA_LABEL_MUTED},
      {"audioInputMuteButtonAriaLabelMutedByHardwareSwitch",
       IDS_SETTINGS_AUDIO_INPUT_MUTE_BUTTON_ARIA_LABEL_MUTED_BY_HARDWARE_SWITCH},
      {"audioInputMuteButtonAriaLabelNotMuted",
       IDS_SETTINGS_AUDIO_INPUT_MUTE_BUTTON_ARIA_LABEL_NOT_MUTED},
      {"audioInputNoiseCancellationTitle",
       IDS_SETTINGS_AUDIO_INPUT_NOISE_CANCELLATION_TITLE},
      {"audioInputTitle", IDS_SETTINGS_AUDIO_INPUT_TITLE},
      {"audioMutedByPolicyTooltip", IDS_SETTINGS_AUDIO_MUTED_BY_POLICY_TOOLTIP},
      {"audioMutedExternallyTooltip",
       IDS_SETTINGS_AUDIO_MUTED_EXTERNALLY_TOOLTIP},
      {"audioOutputDeviceTitle", IDS_SETTINGS_AUDIO_OUTPUT_DEVICE_TITLE},
      {"audioOutputTitle", IDS_SETTINGS_AUDIO_OUTPUT_TITLE},
      {"audioOutputMuteButtonAriaLabelMuted",
       IDS_SETTINGS_AUDIO_OUTPUT_MUTE_BUTTON_ARIA_LABEL_MUTED},
      {"audioOutputMuteButtonAriaLabelNotMuted",
       IDS_SETTINGS_AUDIO_OUTPUT_MUTE_BUTTON_ARIA_LABEL_NOT_MUTED},
      {"audioTitle", IDS_SETTINGS_AUDIO_TITLE},
      {"audioToggleToMuteTooltip", IDS_SETTINGS_AUDIO_TOGGLE_TO_MUTE_TOOLTIP},
      {"audioToggleToUnmuteTooltip",
       IDS_SETTINGS_AUDIO_TOGGLE_TO_UNMUTE_TOOLTIP},
      {"audioVolumeTitle", IDS_SETTINGS_AUDIO_VOLUME_TITLE},
      {"chargingSoundsLabel",
       IDS_SETTINGS_AUDIO_DEVICE_SOUNDS_CHARGING_SOUNDS_LABEL},
      {"deviceStartupSoundLabel",
       IDS_SETTINGS_AUDIO_DEVICE_SOUNDS_STARTUP_SOUND_LABEL},
      {"deviceSoundsTitle", IDS_SETTINGS_AUDIO_DEVICE_SOUNDS_TITLE},
      {"lowBatterySoundLabel",
       IDS_SETTINGS_AUDIO_DEVICE_SOUNDS_LOW_BATTERY_SOUND_LABEL},
  };

  html_source->AddLocalizedStrings(kAudioStrings);

  html_source->AddBoolean("areSystemSoundsEnabled",
                          ash::features::AreSystemSoundsEnabled());
}

void AddDevicePowerStrings(content::WebUIDataSource* html_source) {
  const bool kIsRevampEnabled =
      ash::features::IsOsSettingsRevampWayfindingEnabled();

  webui::LocalizedString kPowerStrings[] = {
      {"calculatingPower", IDS_SETTINGS_POWER_SOURCE_CALCULATING},
      {"powerAdaptiveChargingLabel",
       IDS_SETTINGS_POWER_ADAPTIVE_CHARGING_LABEL},
      {"powerAdaptiveChargingSubtext",
       IDS_SETTINGS_POWER_ADAPTIVE_CHARGING_SUBTEXT},
      {"powerIdleDisplayOff", IDS_SETTINGS_POWER_IDLE_DISPLAY_OFF},
      {"powerIdleDisplayOffSleep", IDS_SETTINGS_POWER_IDLE_DISPLAY_OFF_SLEEP},
      {"powerIdleDisplayOn", IDS_SETTINGS_POWER_IDLE_DISPLAY_ON},
      {"powerIdleDisplayShutDown", IDS_SETTINGS_POWER_IDLE_SHUT_DOWN},
      {"powerIdleDisplayStopSession", IDS_SETTINGS_POWER_IDLE_STOP_SESSION},
      {"powerIdleLabel", IDS_SETTINGS_POWER_IDLE_LABEL},
      {"powerIdleWhileChargingAriaLabel",
       IDS_SETTINGS_POWER_IDLE_WHILE_CHARGING_ARIA_LABEL},
      {"powerInactiveWhilePluggedInLabel",
       kIsRevampEnabled
           ? IDS_OS_SETTINGS_REVAMP_POWER_INACTIVE_WHILE_PLUGGED_IN_LABEL
           : IDS_SETTINGS_POWER_IDLE_WHILE_CHARGING_LABEL},
      {"powerIdleWhileOnBatteryAriaLabel",
       IDS_SETTINGS_POWER_IDLE_WHILE_ON_BATTERY_ARIA_LABEL},
      {"powerInactiveWhileOnBatteryLabel",
       kIsRevampEnabled
           ? IDS_OS_SETTINGS_REVAMP_POWER_INACTIVE_WHILE_ON_BATTERY_LABEL
           : IDS_SETTINGS_POWER_IDLE_WHILE_ON_BATTERY_LABEL},
      {"powerLidShutDownLabel", IDS_SETTINGS_POWER_LID_CLOSED_SHUT_DOWN_LABEL},
      {"powerLidSignOutLabel", IDS_SETTINGS_POWER_LID_CLOSED_SIGN_OUT_LABEL},
      {"powerLidSleepLabel", IDS_SETTINGS_POWER_LID_CLOSED_SLEEP_LABEL},
      {"powerSourceAcAdapter", IDS_SETTINGS_POWER_SOURCE_AC_ADAPTER},
      {"powerSourceBattery", IDS_SETTINGS_POWER_SOURCE_BATTERY},
      {"powerSourceLabel", IDS_SETTINGS_POWER_SOURCE_LABEL},
      {"powerSourceLowPowerCharger",
       IDS_SETTINGS_POWER_SOURCE_LOW_POWER_CHARGER},
      {"powerTitle", IDS_SETTINGS_POWER_TITLE},
      {"powerBatterySaverLabel", IDS_SETTINGS_POWER_BATTERY_SAVER_LABEL},
      {"powerBatterySaverSubtext", IDS_SETTINGS_POWER_BATTERY_SAVER_SUBTEXT},
  };
  html_source->AddLocalizedStrings(kPowerStrings);

  html_source->AddString(
      "powerAdaptiveChargingLearnMoreUrl",
      u"https://support.google.com/chromebook/?p=settings_adaptive_charging");

  // TODO(b:278957245): create and link to real "learn more" webpage.
  html_source->AddString("powerBatterySaverLearnMoreUrl", "about://blank");
}

// Mirrors enum of the same name in enums.xml.
enum class TouchpadSensitivity {
  kNONE = 0,
  kSlowest = 1,
  kSlow = 2,
  kMedium = 3,
  kFast = 4,
  kFastest = 5,
  kMaxValue = kFastest,
};

}  // namespace

DeviceSection::DeviceSection(Profile* profile,
                             SearchTagRegistry* search_tag_registry,
                             PrefService* pref_service)
    : OsSettingsSection(profile, search_tag_registry),
      pref_service_(pref_service) {
  SearchTagRegistry::ScopedTagUpdater updater = registry()->StartUpdate();
  updater.AddSearchTags(GetDeviceSearchConcepts());

  if (ash::features::IsInputDeviceSettingsSplitEnabled()) {
    updater.AddSearchTags(GetPerDeviceKeyboardSearchConcepts());
  } else {
    updater.AddSearchTags(GetKeyboardSearchConcepts());
  }
  if (ShouldShowExternalStorageSettings(profile)) {
    updater.AddSearchTags(GetExternalStorageSearchConcepts());
  }

  // Only when the feature is enabled, the toggle buttons for charging sounds
  // and the low battery sound will be shown up.
  if (ash::features::AreSystemSoundsEnabled()) {
    updater.AddSearchTags(GetAudioPowerSoundsSearchConcepts());
  }

  chromeos::PowerManagerClient* power_manager_client =
      chromeos::PowerManagerClient::Get();
  if (power_manager_client) {
    power_manager_client->AddObserver(this);

    const absl::optional<power_manager::PowerSupplyProperties>& last_status =
        power_manager_client->GetLastStatus();
    if (last_status) {
      PowerChanged(*last_status);
    }

    // Determine whether to show laptop lid power settings.
    power_manager_client->GetSwitchStates(base::BindOnce(
        &DeviceSection::OnGotSwitchStates, weak_ptr_factory_.GetWeakPtr()));

    // Surface adaptive charging setting in search if the feature is enabled.
    if (ash::features::IsAdaptiveChargingEnabled() &&
        Shell::Get()
            ->adaptive_charging_controller()
            ->IsAdaptiveChargingSupported()) {
      updater.AddSearchTags(GetPowerWithAdaptiveChargingSearchConcepts());
    }

    const auto* battery_saver_controller =
        Shell::Get()->battery_saver_controller();
    if (battery_saver_controller != nullptr &&
        battery_saver_controller->IsBatterySaverSupported() &&
        ash::features::IsBatterySaverAvailable()) {
      updater.AddSearchTags(GetPowerWithBatterySaverModeSearchConcepts());
    }
  }

  // Keyboard/mouse search tags are added/removed dynamically.
  pointer_device_observer_.Init();
  pointer_device_observer_.AddObserver(this);
  pointer_device_observer_.CheckDevices();

  // Stylus search tags are added/removed dynamically.
  ui::DeviceDataManager::GetInstance()->AddObserver(this);
  UpdateStylusSearchTags();

  // Display search tags are added/removed dynamically.
  BindCrosDisplayConfigController(
      cros_display_config_.BindNewPipeAndPassReceiver());
  mojo::PendingAssociatedRemote<crosapi::mojom::CrosDisplayConfigObserver>
      observer;
  cros_display_config_observer_receiver_.Bind(
      observer.InitWithNewEndpointAndPassReceiver());
  cros_display_config_->AddObserver(std::move(observer));
  OnDisplayConfigChanged();

  // Night Light settings are added/removed dynamically.
  NightLightController* night_light_controller =
      NightLightController::GetInstance();
  if (night_light_controller) {
    NightLightController::GetInstance()->AddObserver(this);
    OnNightLightEnabledChanged(
        NightLightController::GetInstance()->GetEnabled());
  }
}

DeviceSection::~DeviceSection() {
  pointer_device_observer_.RemoveObserver(this);
  ui::DeviceDataManager::GetInstance()->RemoveObserver(this);

  chromeos::PowerManagerClient* power_manager_client =
      chromeos::PowerManagerClient::Get();
  if (power_manager_client) {
    power_manager_client->RemoveObserver(this);
  }

  NightLightController* night_light_controller =
      NightLightController::GetInstance();
  if (night_light_controller) {
    night_light_controller->RemoveObserver(this);
  }
}

void DeviceSection::AddLoadTimeData(content::WebUIDataSource* html_source) {
  const bool kIsRevampEnabled =
      ash::features::IsOsSettingsRevampWayfindingEnabled();

  webui::LocalizedString kDeviceStrings[] = {
      {"devicePageTitle", IDS_SETTINGS_DEVICE_TITLE},
      {"touchpadScrollLabel",
       kIsRevampEnabled ? IDS_OS_SETTINGS_REVAMP_TOUCHPAD_REVERSE_SCROLL_LABEL
                        : IDS_OS_SETTINGS_TOUCHPAD_REVERSE_SCROLL_LABEL},
      {"touchpadScrollDescription",
       IDS_OS_SETTINGS_REVAMP_TOUCHPAD_REVERSE_SCROLL_DESCRIPTION},
  };
  html_source->AddLocalizedStrings(kDeviceStrings);

  html_source->AddBoolean("isDemoSession", DemoSession::IsDeviceInDemoMode());

  html_source->AddBoolean(
      "enableInputDeviceSettingsSplit",
      base::FeatureList::IsEnabled(ash::features::kInputDeviceSettingsSplit));

  html_source->AddBoolean(
      "enablePeripheralCustomization",
      base::FeatureList::IsEnabled(ash::features::kPeripheralCustomization));

  html_source->AddBoolean("enableAltClickAndSixPackCustomization",
                          base::FeatureList::IsEnabled(
                              ash::features::kAltClickAndSixPackCustomization));

  AddDevicePointersStrings(html_source);
  AddDeviceGraphicsTabletStrings(html_source);
  AddCustomizeButtonsPageStrings(html_source);
  AddDeviceKeyboardStrings(html_source);
  AddDeviceStylusStrings(html_source);
  AddDeviceDisplayStrings(html_source);
  AddDeviceAudioStrings(html_source);
  AddDeviceStorageStrings(html_source,
                          ShouldShowExternalStorageSettings(profile()));
  AddDevicePowerStrings(html_source);

  html_source->AddBoolean("isAdaptiveChargingEnabled",
                          ash::features::IsAdaptiveChargingEnabled() &&
                              Shell::Get()
                                  ->adaptive_charging_controller()
                                  ->IsAdaptiveChargingSupported());
}

void DeviceSection::AddHandlers(content::WebUI* web_ui) {
  web_ui->AddMessageHandler(std::make_unique<DisplayHandler>());
  web_ui->AddMessageHandler(std::make_unique<KeyboardHandler>());
  web_ui->AddMessageHandler(std::make_unique<PointerHandler>());
  web_ui->AddMessageHandler(std::make_unique<PowerHandler>(pref_service_));
  web_ui->AddMessageHandler(std::make_unique<StylusHandler>());
}

int DeviceSection::GetSectionNameMessageId() const {
  return IDS_SETTINGS_DEVICE_TITLE;
}

mojom::Section DeviceSection::GetSection() const {
  return mojom::Section::kDevice;
}

mojom::SearchResultIcon DeviceSection::GetSectionIcon() const {
  return mojom::SearchResultIcon::kLaptop;
}

const char* DeviceSection::GetSectionPath() const {
  return mojom::kDeviceSectionPath;
}

bool DeviceSection::LogMetric(mojom::Setting setting,
                              base::Value& value) const {
  switch (setting) {
    case mojom::Setting::kTouchpadSpeed:
      base::UmaHistogramEnumeration(
          "ChromeOS.Settings.Device.TouchpadSpeedValue",
          static_cast<TouchpadSensitivity>(value.GetInt()));
      return true;

    case mojom::Setting::kKeyboardFunctionKeys:
      base::UmaHistogramBoolean("ChromeOS.Settings.Device.KeyboardFunctionKeys",
                                value.GetBool());
      return true;

    case mojom::Setting::kLowBatterySound:
      base::UmaHistogramBoolean(
          "ChromeOS.Settings.Device.LowBatterySoundButtonEnabled",
          value.GetBool());
      return true;

    case mojom::Setting::kChargingSounds:
      base::UmaHistogramBoolean(
          "ChromeOS.Settings.Device.ChargingSoundsButtonEnabled",
          value.GetBool());
      return true;

    default:
      return false;
  }
}

void DeviceSection::RegisterHierarchy(HierarchyGenerator* generator) const {
  // Pointers.
  generator->RegisterTopLevelSubpage(
      IDS_SETTINGS_MOUSE_AND_TOUCHPAD_TITLE, mojom::Subpage::kPointers,
      mojom::SearchResultIcon::kMouse, mojom::SearchResultDefaultRank::kMedium,
      mojom::kPointersSubpagePath);
  static constexpr mojom::Setting kPointersSettings[] = {
      mojom::Setting::kTouchpadTapToClick,
      mojom::Setting::kTouchpadTapDragging,
      mojom::Setting::kTouchpadReverseScrolling,
      mojom::Setting::kTouchpadAcceleration,
      mojom::Setting::kTouchpadScrollAcceleration,
      mojom::Setting::kTouchpadSpeed,
      mojom::Setting::kTouchpadHapticFeedback,
      mojom::Setting::kTouchpadHapticClickSensitivity,
      mojom::Setting::kPointingStickSwapPrimaryButtons,
      mojom::Setting::kPointingStickSpeed,
      mojom::Setting::kPointingStickAcceleration,
      mojom::Setting::kMouseSwapPrimaryButtons,
      mojom::Setting::kMouseReverseScrolling,
      mojom::Setting::kMouseAcceleration,
      mojom::Setting::kMouseScrollAcceleration,
      mojom::Setting::kMouseSpeed,
  };
  RegisterNestedSettingBulk(mojom::Subpage::kPointers, kPointersSettings,
                            generator);

  if (base::FeatureList::IsEnabled(ash::features::kInputDeviceSettingsSplit)) {
    // Per-device Keyboard.
    generator->RegisterTopLevelSubpage(IDS_SETTINGS_KEYBOARD_TITLE,
                                       mojom::Subpage::kPerDeviceKeyboard,
                                       mojom::SearchResultIcon::kKeyboard,
                                       mojom::SearchResultDefaultRank::kMedium,
                                       mojom::kPerDeviceKeyboardSubpagePath);

    generator->RegisterNestedSubpage(
        IDS_SETTINGS_KEYBOARD_REMAP_KEYS_ROW_LABEL,
        mojom::Subpage::kPerDeviceKeyboardRemapKeys,
        mojom::Subpage::kPerDeviceKeyboard, mojom::SearchResultIcon::kKeyboard,
        mojom::SearchResultDefaultRank::kMedium,
        mojom::kPerDeviceKeyboardRemapKeysSubpagePath);

    static constexpr mojom::Setting kPerDeviceKeyboardSettings[] = {
        mojom::Setting::kKeyboardBlockMetaFkeyRewrites,
        mojom::Setting::kKeyboardRemapKeys,
    };
    RegisterNestedSettingBulk(mojom::Subpage::kPerDeviceKeyboard,
                              kPerDeviceKeyboardSettings, generator);

    // Per-device Mouse.
    generator->RegisterTopLevelSubpage(IDS_SETTINGS_MOUSE_TITLE,
                                       mojom::Subpage::kPerDeviceMouse,
                                       mojom::SearchResultIcon::kMouse,
                                       mojom::SearchResultDefaultRank::kMedium,
                                       mojom::kPerDeviceMouseSubpagePath);

    // Per-device Touchpad.
    generator->RegisterTopLevelSubpage(IDS_SETTINGS_TOUCHPAD_TITLE,
                                       mojom::Subpage::kPerDeviceTouchpad,
                                       mojom::SearchResultIcon::kDisplay,
                                       mojom::SearchResultDefaultRank::kMedium,
                                       mojom::kPerDeviceTouchpadSubpagePath);

    // Per-device Pointing stick.
    generator->RegisterTopLevelSubpage(
        IDS_SETTINGS_POINTING_STICK_TITLE,
        mojom::Subpage::kPerDevicePointingStick,
        mojom::SearchResultIcon::kDisplay,
        mojom::SearchResultDefaultRank::kMedium,
        mojom::kPerDevicePointingStickSubpagePath);
  }

  if (base::FeatureList::IsEnabled(ash::features::kPeripheralCustomization)) {
    // TODO(yyhyyh@): Add icon for graphics tablet to replace the temporary
    // stylus icon.
    generator->RegisterTopLevelSubpage(IDS_SETTINGS_GRAPHICS_TABLET_TITLE,
                                       mojom::Subpage::kGraphicsTablet,
                                       mojom::SearchResultIcon::kStylus,
                                       mojom::SearchResultDefaultRank::kMedium,
                                       mojom::kGraphicsTabletSubpagePath);

    generator->RegisterNestedSubpage(IDS_SETTINGS_CUSTOMIZE_MOUSE_BUTTONS_TITLE,
                                     mojom::Subpage::kCustomizeMouseButtons,
                                     mojom::Subpage::kPerDeviceMouse,
                                     mojom::SearchResultIcon::kMouse,
                                     mojom::SearchResultDefaultRank::kMedium,
                                     mojom::kCustomizeMouseButtonsSubpagePath);

    // TODO(yyhyyh@): Add icon for graphics tablet to replace the temporary
    // stylus icon.
    generator->RegisterNestedSubpage(
        IDS_SETTINGS_GRAPHICS_TABLET_CUSTOMIZE_TABLET_BUTTONS_LABEL,
        mojom::Subpage::kCustomizeTabletButtons,
        mojom::Subpage::kGraphicsTablet, mojom::SearchResultIcon::kStylus,
        mojom::SearchResultDefaultRank::kMedium,
        mojom::kCustomizeTabletButtonsSubpagePath);

    // TODO(yyhyyh@): Decide whether to use stylus icon or add a new icon.
    generator->RegisterNestedSubpage(
        IDS_SETTINGS_GRAPHICS_TABLET_CUSTOMIZE_TABLET_BUTTONS_LABEL,
        mojom::Subpage::kCustomizePenButtons, mojom::Subpage::kGraphicsTablet,
        mojom::SearchResultIcon::kStylus,
        mojom::SearchResultDefaultRank::kMedium,
        mojom::kCustomizePenButtonsSubpagePath);
  }

  // Keyboard.
  generator->RegisterTopLevelSubpage(
      IDS_SETTINGS_KEYBOARD_TITLE, mojom::Subpage::kKeyboard,
      mojom::SearchResultIcon::kKeyboard,
      mojom::SearchResultDefaultRank::kMedium, mojom::kKeyboardSubpagePath);
  static constexpr mojom::Setting kKeyboardSettings[] = {
      mojom::Setting::kShowDiacritic,
      mojom::Setting::kKeyboardFunctionKeys,
      mojom::Setting::kKeyboardAutoRepeat,
      mojom::Setting::kKeyboardShortcuts,
  };
  RegisterNestedSettingBulk(mojom::Subpage::kKeyboard, kKeyboardSettings,
                            generator);

  // Stylus.
  generator->RegisterTopLevelSubpage(
      IDS_SETTINGS_STYLUS_TITLE, mojom::Subpage::kStylus,
      mojom::SearchResultIcon::kStylus, mojom::SearchResultDefaultRank::kMedium,
      mojom::kStylusSubpagePath);
  static constexpr mojom::Setting kStylusSettings[] = {
      mojom::Setting::kStylusToolsInShelf,
      mojom::Setting::kStylusNoteTakingApp,
      mojom::Setting::kStylusNoteTakingFromLockScreen,
      mojom::Setting::kStylusLatestNoteOnLockScreen,
  };
  RegisterNestedSettingBulk(mojom::Subpage::kStylus, kStylusSettings,
                            generator);

  // Display.
  generator->RegisterTopLevelSubpage(
      IDS_SETTINGS_DISPLAY_TITLE, mojom::Subpage::kDisplay,
      mojom::SearchResultIcon::kDisplay,
      mojom::SearchResultDefaultRank::kMedium, mojom::kDisplaySubpagePath);
  static constexpr mojom::Setting kDisplaySettings[] = {
      mojom::Setting::kDisplaySize,
      mojom::Setting::kNightLight,
      mojom::Setting::kDisplayOrientation,
      mojom::Setting::kDisplayArrangement,
      mojom::Setting::kDisplayResolution,
      mojom::Setting::kDisplayRefreshRate,
      mojom::Setting::kDisplayMirroring,
      mojom::Setting::kAllowWindowsToSpanDisplays,
      mojom::Setting::kAmbientColors,
      mojom::Setting::kTouchscreenCalibration,
      mojom::Setting::kNightLightColorTemperature,
      mojom::Setting::kDisplayOverscan,
  };
  RegisterNestedSettingBulk(mojom::Subpage::kDisplay, kDisplaySettings,
                            generator);

  // Storage.
  generator->RegisterTopLevelSubpage(
      IDS_SETTINGS_STORAGE_TITLE, mojom::Subpage::kStorage,
      mojom::SearchResultIcon::kHardDrive,
      mojom::SearchResultDefaultRank::kMedium, mojom::kStorageSubpagePath);
  generator->RegisterNestedSubpage(
      IDS_SETTINGS_STORAGE_EXTERNAL, mojom::Subpage::kExternalStorage,
      mojom::Subpage::kStorage, mojom::SearchResultIcon::kHardDrive,
      mojom::SearchResultDefaultRank::kMedium,
      mojom::kExternalStorageSubpagePath);

  // Audio.
  generator->RegisterTopLevelSubpage(
      IDS_SETTINGS_AUDIO_TITLE, mojom::Subpage::kAudio,
      mojom::SearchResultIcon::kAudio, mojom::SearchResultDefaultRank::kMedium,
      mojom::kAudioSubpagePath);
  generator->RegisterNestedSetting(mojom::Setting::kChargingSounds,
                                   mojom::Subpage::kAudio);
  generator->RegisterNestedSetting(mojom::Setting::kLowBatterySound,
                                   mojom::Subpage::kAudio);

  // Power.
  generator->RegisterTopLevelSubpage(
      IDS_SETTINGS_POWER_TITLE, mojom::Subpage::kPower,
      mojom::SearchResultIcon::kPower, mojom::SearchResultDefaultRank::kMedium,
      mojom::kPowerSubpagePath);
  static constexpr mojom::Setting kPowerSettings[] = {
      mojom::Setting::kPowerIdleBehaviorWhileCharging,
      mojom::Setting::kPowerIdleBehaviorWhileOnBattery,
      mojom::Setting::kPowerSource,
      mojom::Setting::kSleepWhenLaptopLidClosed,
      mojom::Setting::kAdaptiveCharging,
      mojom::Setting::kBatterySaver,
  };
  RegisterNestedSettingBulk(mojom::Subpage::kPower, kPowerSettings, generator);
}

void DeviceSection::TouchpadExists(bool exists) {
  SearchTagRegistry::ScopedTagUpdater updater = registry()->StartUpdate();

  if (!ash::features::IsInputDeviceSettingsSplitEnabled()) {
    updater.RemoveSearchTags(GetTouchpadSearchConcepts());

    if (exists) {
      updater.AddSearchTags(GetTouchpadSearchConcepts());
    }
    return;
  }

  updater.RemoveSearchTags(GetPerDeviceTouchpadSearchConcepts());

  if (exists) {
    updater.AddSearchTags(GetPerDeviceTouchpadSearchConcepts());
  }
}

void DeviceSection::HapticTouchpadExists(bool exists) {
  SearchTagRegistry::ScopedTagUpdater updater = registry()->StartUpdate();

  if (!ash::features::IsInputDeviceSettingsSplitEnabled()) {
    updater.RemoveSearchTags(GetTouchpadHapticSearchConcepts());

    if (exists) {
      updater.AddSearchTags(GetTouchpadHapticSearchConcepts());
    }
    return;
  }

  updater.RemoveSearchTags(GetPerDeviceTouchpadHapticSearchConcepts());

  if (exists) {
    updater.AddSearchTags(GetPerDeviceTouchpadHapticSearchConcepts());
  }
}

void DeviceSection::MouseExists(bool exists) {
  SearchTagRegistry::ScopedTagUpdater updater = registry()->StartUpdate();

  if (!ash::features::IsInputDeviceSettingsSplitEnabled()) {
    updater.RemoveSearchTags(GetMouseSearchConcepts());
    updater.RemoveSearchTags(GetMouseScrollAccelerationSearchConcepts());

    if (exists) {
      updater.AddSearchTags(GetMouseSearchConcepts());
      if (features::IsAllowScrollSettingsEnabled()) {
        updater.AddSearchTags(GetMouseScrollAccelerationSearchConcepts());
      }
    }
    return;
  }

  updater.RemoveSearchTags(GetPerDeviceMouseSearchConcepts());
  updater.RemoveSearchTags(GetPerDeviceMouseScrollAccelerationSearchConcepts());

  if (exists) {
    updater.AddSearchTags(GetPerDeviceMouseSearchConcepts());
    if (features::IsAllowScrollSettingsEnabled()) {
      updater.AddSearchTags(
          GetPerDeviceMouseScrollAccelerationSearchConcepts());
    }
  }
}

void DeviceSection::PointingStickExists(bool exists) {
  SearchTagRegistry::ScopedTagUpdater updater = registry()->StartUpdate();

  if (!ash::features::IsInputDeviceSettingsSplitEnabled()) {
    updater.RemoveSearchTags(GetPointingStickSearchConcepts());

    if (exists) {
      updater.AddSearchTags(GetPointingStickSearchConcepts());
    }
    return;
  }

  updater.RemoveSearchTags(GetPerDevicePointingStickSearchConcepts());

  if (exists) {
    updater.AddSearchTags(GetPerDevicePointingStickSearchConcepts());
  }
}

void DeviceSection::OnDeviceListsComplete() {
  UpdateStylusSearchTags();
}

void DeviceSection::OnNightLightEnabledChanged(bool enabled) {
  OnDisplayConfigChanged();
}

void DeviceSection::OnDisplayConfigChanged() {
  cros_display_config_->GetDisplayUnitInfoList(
      /*single_unified=*/true,
      base::BindOnce(&DeviceSection::OnGetDisplayUnitInfoList,
                     base::Unretained(this)));
}

void DeviceSection::PowerChanged(
    const power_manager::PowerSupplyProperties& properties) {
  SearchTagRegistry::ScopedTagUpdater updater = registry()->StartUpdate();

  if (properties.battery_state() !=
      power_manager::PowerSupplyProperties_BatteryState_NOT_PRESENT) {
    updater.AddSearchTags(GetPowerWithBatterySearchConcepts());
  }
}

void DeviceSection::OnGetDisplayUnitInfoList(
    std::vector<crosapi::mojom::DisplayUnitInfoPtr> display_unit_info_list) {
  cros_display_config_->GetDisplayLayoutInfo(base::BindOnce(
      &DeviceSection::OnGetDisplayLayoutInfo, base::Unretained(this),
      std::move(display_unit_info_list)));
}

void DeviceSection::OnGetDisplayLayoutInfo(
    std::vector<crosapi::mojom::DisplayUnitInfoPtr> display_unit_info_list,
    crosapi::mojom::DisplayLayoutInfoPtr display_layout_info) {
  bool has_multiple_displays = display_unit_info_list.size() > 1u;

  // Mirroring mode is active if there's at least one display and if there's a
  // mirror source ID.
  bool is_mirrored = !display_unit_info_list.empty() &&
                     display_layout_info->mirror_source_id.has_value();

  bool has_internal_display = false;
  bool has_external_display = false;
  bool unified_desktop_mode = false;
  for (const auto& display_unit_info : display_unit_info_list) {
    has_internal_display |= display_unit_info->is_internal;
    has_external_display |= !display_unit_info->is_internal;

    unified_desktop_mode |= display_unit_info->is_primary &&
                            display_layout_info->layout_mode ==
                                crosapi::mojom::DisplayLayoutMode::kUnified;
  }

  SearchTagRegistry::ScopedTagUpdater updater = registry()->StartUpdate();

  // Arrangement UI.
  if (has_multiple_displays || is_mirrored) {
    updater.AddSearchTags(GetDisplayArrangementSearchConcepts());
  } else {
    updater.RemoveSearchTags(GetDisplayArrangementSearchConcepts());
  }

  // Mirror toggle.
  if (is_mirrored || (!unified_desktop_mode && has_multiple_displays)) {
    updater.AddSearchTags(GetDisplayMirrorSearchConcepts());
  } else {
    updater.RemoveSearchTags(GetDisplayMirrorSearchConcepts());
  }

  // Unified Desktop toggle.
  if (unified_desktop_mode ||
      (IsUnifiedDesktopAvailable() && has_multiple_displays && !is_mirrored)) {
    updater.AddSearchTags(GetDisplayUnifiedDesktopSearchConcepts());
  } else {
    updater.RemoveSearchTags(GetDisplayUnifiedDesktopSearchConcepts());
  }

  // External display settings.
  if (has_external_display) {
    updater.AddSearchTags(GetDisplayExternalSearchConcepts());
  } else {
    updater.RemoveSearchTags(GetDisplayExternalSearchConcepts());
  }

  // Refresh Rate dropdown.
  if (has_external_display && IsListAllDisplayModesEnabled()) {
    updater.AddSearchTags(GetDisplayExternalWithRefreshSearchConcepts());
  } else {
    updater.RemoveSearchTags(GetDisplayExternalWithRefreshSearchConcepts());
  }

  // Orientation settings.
  if (!unified_desktop_mode) {
    updater.AddSearchTags(GetDisplayOrientationSearchConcepts());
  } else {
    updater.RemoveSearchTags(GetDisplayOrientationSearchConcepts());
  }

  // Ambient color settings.
  if (DoesDeviceSupportAmbientColor() && has_internal_display) {
    updater.AddSearchTags(GetDisplayAmbientSearchConcepts());
  } else {
    updater.RemoveSearchTags(GetDisplayAmbientSearchConcepts());
  }

  // Touch calibration settings.
  if (IsTouchCalibrationAvailable()) {
    updater.AddSearchTags(GetDisplayTouchCalibrationSearchConcepts());
  } else {
    updater.RemoveSearchTags(GetDisplayTouchCalibrationSearchConcepts());
  }

  // Night Light on settings.
  if (NightLightController::GetInstance()->GetEnabled()) {
    updater.AddSearchTags(GetDisplayNightLightOnSearchConcepts());
  } else {
    updater.RemoveSearchTags(GetDisplayNightLightOnSearchConcepts());
  }
}

void DeviceSection::OnGotSwitchStates(
    absl::optional<chromeos::PowerManagerClient::SwitchStates> result) {
  SearchTagRegistry::ScopedTagUpdater updater = registry()->StartUpdate();

  if (result && result->lid_state !=
                    chromeos::PowerManagerClient::LidState::NOT_PRESENT) {
    updater.AddSearchTags(GetPowerWithLaptopLidSearchConcepts());
  }
}

void DeviceSection::UpdateStylusSearchTags() {
  // If not yet complete, wait for OnDeviceListsComplete() callback.
  if (!ui::DeviceDataManager::GetInstance()->AreDeviceListsComplete()) {
    return;
  }

  SearchTagRegistry::ScopedTagUpdater updater = registry()->StartUpdate();

  // TODO(https://crbug.com/1071905): Only show stylus settings if a stylus has
  // been set up. HasStylusInput() will return true for any stylus-compatible
  // device, even if it doesn't have a stylus.
  if (stylus_utils::HasStylusInput()) {
    updater.AddSearchTags(GetStylusSearchConcepts());
  } else {
    updater.RemoveSearchTags(GetStylusSearchConcepts());
  }
}

void DeviceSection::AddDevicePointersStrings(
    content::WebUIDataSource* html_source) {
  const bool kIsRevampEnabled =
      ash::features::IsOsSettingsRevampWayfindingEnabled();
  const bool kIsAllowMouseScrollSettingsEnabled =
      features::IsAllowScrollSettingsEnabled();

  webui::LocalizedString kPointersStrings[] = {
      {"allMiceDisconnectedA11yLabel",
       IDS_SETTINGS_PER_DEVICE_ALL_MICE_DISCONNECTED_A11Y_LABEL},
      {"allTouchpadsDisconnectedA11yLabel",
       IDS_SETTINGS_PER_DEVICE_ALL_TOUCHPADS_DISCONNECTED_A11Y_LABEL},
      {"allPointingSticksDisconnectedA11yLabel",
       IDS_SETTINGS_PER_DEVICE_ALL_POINTING_STICKS_DISCONNECTED_A11Y_LABEL},
      {"deviceConnectedA11yLabel",
       IDS_SETTINGS_PER_DEVICE_CONNECTED_A11Y_LABEL},
      {"deviceDisconnectedA11yLabel",
       IDS_SETTINGS_PER_DEVICE_DISCONNECTED_A11Y_LABEL},
      {"mouseTitle", IDS_SETTINGS_MOUSE_TITLE},
      {"builtInPointingStickName", IDS_SETTINGS_BUILT_IN_POINTING_STICK_NAME},
      {"pointingStickTitle", IDS_SETTINGS_POINTING_STICK_TITLE},
      {"builtInTouchpadName", IDS_SETTINGS_BUILT_IN_TOUCHPAD_NAME},
      {"touchpadTitle", IDS_SETTINGS_TOUCHPAD_TITLE},
      {"mouseAndTouchpadTitle", IDS_SETTINGS_MOUSE_AND_TOUCHPAD_TITLE},
      {"touchpadTapToClickEnabledLabel",
       kIsRevampEnabled ? IDS_OS_SETTINGS_REVAMP_TOUCHPAD_TAP_TO_CLICK_LABEL
                        : IDS_SETTINGS_TOUCHPAD_TAP_TO_CLICK_ENABLED_LABEL},
      {"touchpadTapToClickDescription",
       IDS_OS_SETTINGS_REVAMP_TOUCHPAD_TAP_TO_CLICK_DESCRIPTION},
      {"touchpadSpeed", IDS_SETTINGS_TOUCHPAD_SPEED_LABEL},
      {"pointerSlow", IDS_SETTINGS_POINTER_SPEED_SLOW_LABEL},
      {"pointerFast", IDS_SETTINGS_POINTER_SPEED_FAST_LABEL},
      {"mouseScrollSpeed", IDS_SETTINGS_MOUSE_SCROLL_SPEED_LABEL},
      {"mouseSpeed", IDS_SETTINGS_MOUSE_SPEED_LABEL},
      {"cursorSpeed", IDS_SETTINGS_CURSOR_SPEED_LABEL},
      {"pointingStickSpeed", IDS_SETTINGS_POINTING_STICK_SPEED_LABEL},
      {"mouseSwapButtonsLabel",
       kIsRevampEnabled ? IDS_OS_SETTINGS_REVAMP_MOUSE_SWAP_BUTTONS_LABEL
                        : IDS_SETTINGS_MOUSE_SWAP_BUTTONS_LABEL},
      {"mouseCursor", IDS_SETTINGS_MOUSE_CURSOR_LABEL},
      {"mouseScrolling", IDS_SETTINGS_MOUSE_SCROLLING_LABEL},
      {"pointingStickPrimaryButton",
       IDS_SETTINGS_POINTING_STICK_PRIMARY_BUTTON_LABEL},
      {"primaryMouseButtonLeft", IDS_SETTINGS_PRIMARY_MOUSE_BUTTON_LEFT_LABEL},
      {"primaryMouseButtonRight",
       IDS_SETTINGS_PRIMARY_MOUSE_BUTTON_RIGHT_LABEL},
      {"mouseReverseScrollLabel",
       (kIsRevampEnabled || kIsAllowMouseScrollSettingsEnabled)
           ? IDS_OS_SETTINGS_REVAMP_MOUSE_REVERSE_SCROLL_LABEL
           : IDS_SETTINGS_MOUSE_REVERSE_SCROLL_LABEL},
      {"mouseReverseScrollDescription",
       IDS_OS_SETTINGS_REVAMP_MOUSE_REVERSE_SCROLL_DESCRIPTION},
      {"mouseAccelerationLabel",
       kIsRevampEnabled ? IDS_OS_SETTINGS_REVAMP_MOUSE_ACCELERATION_LABEL
                        : IDS_SETTINGS_MOUSE_ACCELERATION_LABEL},
      {"mouseAccelerationDescription",
       IDS_OS_SETTINGS_REVAMP_MOUSE_ACCELERATION_DESCRIPTION},
      {"cursorAccelerationLabel", IDS_SETTINGS_CURSOR_ACCELERATION_LABEL},
      {"mouseScrollAccelerationLabel",
       IDS_SETTINGS_MOUSE_SCROLL_ACCELERATION_LABEL},
      {"pointingStickAccelerationLabel",
       IDS_SETTINGS_POINTING_STICK_ACCELERATION_LABEL},
      {"touchpadAccelerationLabel",
       kIsRevampEnabled ? IDS_OS_SETTINGS_REVAMP_TOUCHPAD_ACCELERATION_LABEL
                        : IDS_SETTINGS_TOUCHPAD_ACCELERATION_LABEL},
      {"touchpadAccelerationDescription",
       IDS_OS_SETTINGS_REVAMP_TOUCHPAD_ACCELERATION_DESCRIPTION},
      {"touchpadHapticClickSensitivityLabel",
       IDS_SETTINGS_TOUCHPAD_HAPTIC_CLICK_SENSITIVITY_LABEL},
      {"touchpadHapticFeedbackTitle",
       IDS_SETTINGS_TOUCHPAD_HAPTIC_FEEDBACK_TITLE},
      {"touchpadHapticFeedbackSecondaryText",
       IDS_SETTINGS_TOUCHPAD_HAPTIC_FEEDBACK_SECONDARY_TEXT},
      {"touchpadHapticFirmClickLabel",
       IDS_SETTINGS_TOUCHPAD_HAPTIC_FIRM_CLICK_LABEL},
      {"touchpadHapticLightClickLabel",
       IDS_SETTINGS_TOUCHPAD_HAPTIC_LIGHT_CLICK_LABEL},
      {"touchpadSimulateRightClickLabel",
       IDS_SETTINGS_TOUCHPAD_SIMULATE_RIGHT_CLICK_LABEL},
      {"touchpadSimulateRightClickOptionAlt",
       IDS_SETTINGS_TOUCHPAD_SIMULATE_RIGHT_CLICK_OPTION_ALT},
      {"touchpadSimulateRightClickOptionDisabled",
       IDS_SETTINGS_TOUCHPAD_SIMULATE_RIGHT_CLICK_OPTION_DISABLED},
      {"learnMoreLabel", IDS_SETTINGS_LEARN_MORE_LABEL},
      {"modifierKeysLabel", IDS_SETTINGS_MODIFIER_KEYS_LABEL},
      {"otherKeysLabel", IDS_SETTINGS_OTHER_KEYS_LABEL},
      {"sixPackKeyLabelInsert", IDS_SETTINGS_SIX_PACK_KEY_INSERT},
      {"sixPackKeyLabelHome", IDS_SETTINGS_SIX_PACK_KEY_HOME},
      {"sixPackKeyLabelEnd", IDS_SETTINGS_SIX_PACK_KEY_END},
      {"sixPackKeyLabelDelete", IDS_SETTINGS_SIX_PACK_KEY_DELETE},
      {"sixPackKeyLabelPageUp", IDS_SETTINGS_SIX_PACK_KEY_PAGE_UP},
      {"sixPackKeyLabelPageDown", IDS_SETTINGS_SIX_PACK_KEY_PAGE_DOWN},
      {"sixPackKeyDeleteAlt", IDS_SETTINGS_SIX_PACK_KEY_DELETE_ALT},
      {"sixPackKeyHomeAlt", IDS_SETTINGS_SIX_PACK_KEY_HOME_ALT},
      {"sixPackKeyEndAlt", IDS_SETTINGS_SIX_PACK_KEY_END_ALT},
      {"sixPackKeyPageUpAlt", IDS_SETTINGS_SIX_PACK_KEY_PAGE_UP_ALT},
      {"sixPackKeyPageDownAlt", IDS_SETTINGS_SIX_PACK_KEY_PAGE_DOWN_ALT},
      {"sixPackKeyPageDownSearch", IDS_SETTINGS_SIX_PACK_KEY_PAGE_DOWN_SEARCH},
      {"sixPackKeyInsertSearch", IDS_SETTINGS_SIX_PACK_KEY_INSERT_SEARCH},
      {"sixPackKeyDisabled", IDS_SETTINGS_SIX_PACK_KEY_OPTION_DISABLED},
  };
  html_source->AddLocalizedStrings(kPointersStrings);

  html_source->AddString("naturalScrollLearnMoreLink",
                         GetHelpUrlWithBoard(chrome::kNaturalScrollHelpURL));
  html_source->AddString(
      "scrollAccelerationLearnMoreLink",
      GetHelpUrlWithBoard(chrome::kScrollAccelerationHelpURL));
  html_source->AddString("hapticFeedbackLearnMoreLink",
                         GetHelpUrlWithBoard(chrome::kHapticFeedbackHelpURL));

  html_source->AddBoolean("allowScrollSettings",
                          features::IsAllowScrollSettingsEnabled());
}

void DeviceSection::AddDeviceGraphicsTabletStrings(
    content::WebUIDataSource* html_source) const {
  static constexpr webui::LocalizedString kGraphicsTabletStrings[] = {
      {"customizePenButtonsLabel",
       IDS_SETTINGS_GRAPHICS_TABLET_CUSTOMIZE_PEN_BUTTONS_LABEL},
      {"customizeTabletButtonsLabel",
       IDS_SETTINGS_GRAPHICS_TABLET_CUSTOMIZE_TABLET_BUTTONS_LABEL},
      {"tabletTitle", IDS_SETTINGS_GRAPHICS_TABLET_TITLE},
  };
  html_source->AddLocalizedStrings(kGraphicsTabletStrings);
}

void DeviceSection::AddCustomizeButtonsPageStrings(
    content::WebUIDataSource* html_source) const {
  static constexpr webui::LocalizedString kCustomizeButtonsPageStrings[] = {
      {"buttonRemappingDialogInputLabel",
       IDS_SETTINGS_CUSTOMIZE_BUTTONS_RENAMING_DIALOG_INPUT_LABEL},
      {"buttonRemappingDialogCancelLabel",
       IDS_SETTINGS_CUSTOMIZE_BUTTONS_DIALOG_CANCEL},
      {"buttonRemappingDialogSaveLabel",
       IDS_SETTINGS_CUSTOMIZE_BUTTONS_DIALOG_SAVE},
      {"buttonRenamingDialogTitle",
       IDS_SETTINGS_CUSTOMIZE_BUTTONS_RENAMING_DIALOG_TITLE},
      {"customizeMouseButtonsTitle",
       IDS_SETTINGS_CUSTOMIZE_MOUSE_BUTTONS_TITLE},
      {"keyCombinationOptionLabel", IDS_SETTINGS_KEY_COMBINATION_OPTION_LABEL},
      {"noRemappingOptionLabel", IDS_SETTINGS_NO_REMAPPING_OPTION_LABEL},
  };
  html_source->AddLocalizedStrings(kCustomizeButtonsPageStrings);
}

void DeviceSection::AddDeviceDisplayStrings(
    content::WebUIDataSource* html_source) const {
  const bool kIsRevampEnabled =
      ash::features::IsOsSettingsRevampWayfindingEnabled();

  webui::LocalizedString kDisplayStrings[] = {
      {"displayAmbientColorTitle", IDS_SETTINGS_DISPLAY_AMBIENT_COLOR_TITLE},
      {"displayAmbientColorSubtitle",
       IDS_SETTINGS_DISPLAY_AMBIENT_COLOR_SUBTITLE},
      {"displayArrangementTitle", IDS_SETTINGS_DISPLAY_ARRANGEMENT_TITLE},
      {"displayMirror", IDS_SETTINGS_DISPLAY_MIRROR},
      {"displayMirrorDisplayName", IDS_SETTINGS_DISPLAY_MIRROR_DISPLAY_NAME},
      {"displayNightLightLabel", IDS_SETTINGS_DISPLAY_NIGHT_LIGHT_LABEL},
      {"displayNightLightOnAtSunset",
       IDS_SETTINGS_DISPLAY_NIGHT_LIGHT_ON_AT_SUNSET},
      {"displayNightLightOffAtSunrise",
       IDS_SETTINGS_DISPLAY_NIGHT_LIGHT_OFF_AT_SUNRISE},
      {"displayNightLightScheduleCustom",
       IDS_SETTINGS_DISPLAY_NIGHT_LIGHT_SCHEDULE_CUSTOM},
      {"displayNightLightScheduleLabel",
       IDS_SETTINGS_DISPLAY_NIGHT_LIGHT_SCHEDULE_LABEL},
      {"displayNightLightScheduleNever",
       IDS_SETTINGS_DISPLAY_NIGHT_LIGHT_SCHEDULE_NEVER},
      {"displayNightLightScheduleSunsetToSunRise",
       IDS_SETTINGS_DISPLAY_NIGHT_LIGHT_SCHEDULE_SUNSET_TO_SUNRISE},
      {"displayNightLightTemperatureLabel",
       IDS_SETTINGS_DISPLAY_NIGHT_LIGHT_TEMPERATURE_LABEL},
      {"displayNightLightTempSliderMaxLabel",
       IDS_SETTINGS_DISPLAY_NIGHT_LIGHT_TEMP_SLIDER_MAX_LABEL},
      {"displayNightLightTempSliderMinLabel",
       IDS_SETTINGS_DISPLAY_NIGHT_LIGHT_TEMP_SLIDER_MIN_LABEL},
      {"displayNightLightText", IDS_SETTINGS_DISPLAY_NIGHT_LIGHT_TEXT},
      {"displayOrientation", IDS_SETTINGS_DISPLAY_ORIENTATION},
      {"displayOrientationAutoRotate",
       IDS_SETTINGS_DISPLAY_ORIENTATION_AUTO_ROTATE},
      {"displayOrientationStandard", IDS_SETTINGS_DISPLAY_ORIENTATION_STANDARD},
      {"displayOverscanInstructions",
       IDS_SETTINGS_DISPLAY_OVERSCAN_INSTRUCTIONS},
      {"displayOverscanPageText", IDS_SETTINGS_DISPLAY_OVERSCAN_TEXT},
      {"displayOverscanPageTitle",
       kIsRevampEnabled ? IDS_OS_SETTINGS_REVAMP_DISPLAY_BOUNDARIES_TITLE
                        : IDS_SETTINGS_DISPLAY_OVERSCAN_TITLE},
      {"displayOverscanPosition", IDS_SETTINGS_DISPLAY_OVERSCAN_POSITION},
      {"displayOverscanResize", IDS_SETTINGS_DISPLAY_OVERSCAN_RESIZE},
      {"displayOverscanReset", IDS_SETTINGS_DISPLAY_OVERSCAN_RESET},
      {"displayOverscanSubtitle",
       kIsRevampEnabled ? IDS_OS_SETTINGS_REVAMP_DISPLAY_BOUNDARIES_DESCRIPTION
                        : IDS_SETTINGS_DISPLAY_OVERSCAN_SUBTITLE},
      {"displayRefreshRateInterlacedMenuItem",
       IDS_SETTINGS_DISPLAY_REFRESH_RATE_INTERLACED_MENU_ITEM},
      {"displayRefreshRateMenuItem",
       IDS_SETTINGS_DISPLAY_REFRESH_RATE_MENU_ITEM},
      {"displayRefreshRateSublabel",
       kIsRevampEnabled
           ? IDS_OS_SETTINGS_REVAMP_DISPLAY_REFRESH_RATE_DESCRIPTION
           : IDS_SETTINGS_DISPLAY_REFRESH_RATE_SUBLABEL},
      {"displayRefreshRateTitle",
       kIsRevampEnabled ? IDS_OS_SETTINGS_REVAMP_DISPLAY_REFRESH_RATE_TITLE
                        : IDS_SETTINGS_DISPLAY_REFRESH_RATE_TITLE},
      {"displayResolutionInterlacedMenuItem",
       IDS_SETTINGS_DISPLAY_RESOLUTION_INTERLACED_MENU_ITEM},
      {"displayResolutionMenuItem", IDS_SETTINGS_DISPLAY_RESOLUTION_MENU_ITEM},
      {"displayResolutionOnlyMenuItem",
       IDS_SETTINGS_DISPLAY_RESOLUTION_ONLY_MENU_ITEM},
      {"displayResolutionSublabel", IDS_SETTINGS_DISPLAY_RESOLUTION_SUBLABEL},
      {"displayResolutionText", IDS_SETTINGS_DISPLAY_RESOLUTION_TEXT},
      {"displayResolutionTextBest", IDS_SETTINGS_DISPLAY_RESOLUTION_TEXT_BEST},
      {"displayResolutionTextNative",
       IDS_SETTINGS_DISPLAY_RESOLUTION_TEXT_NATIVE},
      {"displayResolutionTitle", IDS_SETTINGS_DISPLAY_RESOLUTION_TITLE},
      {"displayScreenExtended", IDS_SETTINGS_DISPLAY_SCREEN_EXTENDED},
      {"displayScreenPrimary", IDS_SETTINGS_DISPLAY_SCREEN_PRIMARY},
      {"displayScreenTitle", IDS_SETTINGS_DISPLAY_SCREEN},
      {"displaySizeSliderMaxLabel", IDS_SETTINGS_DISPLAY_ZOOM_SLIDER_MAXIMUM},
      {"displaySizeSliderMinLabel", IDS_SETTINGS_DISPLAY_ZOOM_SLIDER_MINIMUM},
      {"displayTitle", kIsRevampEnabled ? IDS_OS_SETTINGS_REVAMP_DISPLAY_TITLE
                                        : IDS_SETTINGS_DISPLAY_TITLE},
      {"displayTouchCalibrationText",
       IDS_SETTINGS_DISPLAY_TOUCH_CALIBRATION_TEXT},
      {"displayTouchCalibrationTitle",
       IDS_SETTINGS_DISPLAY_TOUCH_CALIBRATION_TITLE},
      {"displayUnifiedDesktop", IDS_SETTINGS_DISPLAY_UNIFIED_DESKTOP},
      {"displayUnifiedDesktopOff", IDS_SETTINGS_DISPLAY_UNIFIED_DESKTOP_OFF},
      {"displayUnifiedDesktopOn", IDS_SETTINGS_DISPLAY_UNIFIED_DESKTOP_ON},
      {"displayZoomLogicalResolutionDefaultText",
       IDS_SETTINGS_DISPLAY_ZOOM_LOGICAL_RESOLUTION_DEFAULT_TEXT},
      {"displayZoomLogicalResolutionText",
       IDS_SETTINGS_DISPLAY_ZOOM_LOGICAL_RESOLUTION_TEXT},
      {"displayZoomNativeLogicalResolutionNativeText",
       IDS_SETTINGS_DISPLAY_ZOOM_LOGICAL_RESOLUTION_NATIVE_TEXT},
      {"displayZoomLabel", kIsRevampEnabled
                               ? IDS_OS_SETTINGS_REVAMP_DISPLAY_ZOOM_LABEL
                               : IDS_SETTINGS_DISPLAY_ZOOM_TITLE},
      {"displayZoomDescription",
       kIsRevampEnabled ? IDS_OS_SETTINGS_REVAMP_DISPLAY_ZOOM_DESCRIPTION
                        : IDS_SETTINGS_DISPLAY_ZOOM_SUBLABEL},
      {"displayZoomValue", IDS_SETTINGS_DISPLAY_ZOOM_VALUE},
  };
  html_source->AddLocalizedStrings(kDisplayStrings);

  html_source->AddLocalizedString(
      "displayArrangementText",
      IDS_SETTINGS_DISPLAY_ARRANGEMENT_WITH_KEYBOARD_TEXT);

  html_source->AddBoolean(
      "isCryptohomeDataEphemeral",
      user_manager::UserManager::Get()->IsCurrentUserCryptohomeDataEphemeral());

  html_source->AddBoolean("unifiedDesktopAvailable",
                          IsUnifiedDesktopAvailable());

  html_source->AddBoolean("listAllDisplayModes",
                          IsListAllDisplayModesEnabled());

  html_source->AddBoolean("deviceSupportsAmbientColor",
                          DoesDeviceSupportAmbientColor());

  html_source->AddBoolean("enableForceRespectUiGainsToggle",
                          IsShowForceRespectUiGainsToggleEnabled());

  html_source->AddBoolean("enableTouchCalibrationSetting",
                          IsTouchCalibrationAvailable());

  html_source->AddString("invalidDisplayId",
                         base::NumberToString(display::kInvalidDisplayId));

  html_source->AddBoolean("enableDriveFsBulkPinning",
                          drive::util::IsDriveFsBulkPinningEnabled(profile()));

  html_source->AddBoolean(
      "allowDisplayAlignmentApi",
      base::FeatureList::IsEnabled(ash::features::kDisplayAlignAssist));
}

}  // namespace ash::settings
