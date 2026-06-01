# Copyright 2023 The Chromium Authors
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

from __future__ import annotations

import argparse
import dataclasses
import logging
import re
from typing import TYPE_CHECKING, Any, Dict, List, Optional, cast

from immutabledict import immutabledict

from crossbench import path as pth
from crossbench import plt
from crossbench.cli.config.driver_type import BrowserDriverType
from crossbench.config import ConfigObject, ConfigParser
from crossbench.parse import NumberParser, ObjectParser, PathParser
from crossbench.plt.android_adb import Adb, AndroidAdbPlatform, adb_devices
from crossbench.plt.chromeos_ssh import ChromeOsSshPlatform
from crossbench.plt.ios import ios_devices

if TYPE_CHECKING:
  from crossbench.path import AnyPath, LocalPath


class AmbiguousDriverIdentifier(argparse.ArgumentTypeError):
  pass


IOS_UUID_RE = re.compile(r"[0-9A-Z]+-[0-9A-Z-]+")


def driver_path(
    value: Optional[pth.AnyPathLike],
    type: BrowserDriverType,  #pylint: disable=redefined-builtin
    name: str = "driver path"
) -> Optional[pth.AnyPath]:
  if not value:
    return None
  if type.is_remote_driver:
    return PathParser.any_path(value, name)
  return PathParser.binary_path(value, name)


@dataclasses.dataclass(frozen=True)
class DriverConfig(ConfigObject):
  type: BrowserDriverType = BrowserDriverType.default()
  path: Optional[AnyPath] = None
  device_id: Optional[str] = None
  adb_bin: Optional[AnyPath] = None
  settings: Optional[immutabledict] = None

  @classmethod
  def default(cls) -> DriverConfig:
    return cls(BrowserDriverType.default())

  @classmethod
  def parse_str(cls, value: str) -> DriverConfig:
    if not value:
      raise argparse.ArgumentTypeError("Cannot parse empty string")
    # Variant 1: $PATH
    path: Optional[LocalPath] = pth.try_resolve_existing_path(value)
    driver_type: BrowserDriverType = BrowserDriverType.default()
    if path:
      if path.stat().st_size == 0:
        raise argparse.ArgumentTypeError(f"Driver path is empty file: {path}")
    else:
      if cls.value_has_path_prefix(value):
        raise argparse.ArgumentTypeError(
            f"Driver path does not exist: {repr(value)}")
      if value[0] == "{":
        # Variant 1: full hjson config
        return cls.parse_inline_hjson(value)
      # Variant 2: $DRIVER_TYPE
      try:
        driver_type = BrowserDriverType.parse(value)
      except argparse.ArgumentTypeError as original_error:
        try:
          return cls.parse_short_settings(value, plt.PLATFORM)
        except AmbiguousDriverIdentifier:  # pylint: disable=try-except-raise
          raise
        except ValueError as e:
          logging.debug("Parsing short inline driver config failed: %s", e)
          raise original_error from e
    return DriverConfig(driver_type, path)

  @classmethod
  def parse_short_settings(cls, value: str,
                           platform: plt.Platform) -> DriverConfig:
    """Check for short versions and multiple candidates"""
    logging.debug("Looking for driver candidates: %s", value)
    candidate: Optional[DriverConfig]
    if candidate := cls.try_parse_adb_settings(value, platform):
      return candidate
    if platform.is_macos:
      if candidate := cls.try_parse_ios_settings(value, platform):
        return candidate
    # TODO: add more custom parsing here
    raise ValueError("Unknown setting")

  @classmethod
  def try_parse_adb_settings(cls, value: str,
                             platform: plt.Platform) -> Optional[DriverConfig]:
    candidate_serials: List[str] = []
    pattern: re.Pattern = cls.compile_search_pattern(value)
    for serial, info in adb_devices(platform).items():
      if pattern.fullmatch(serial):
        candidate_serials.append(serial)
        continue
      print(info)
      for key, info_value in info.items():
        if (pattern.fullmatch(f"{key}:{info_value}") or
            pattern.fullmatch(info_value)):
          candidate_serials.append(serial)
          break
    if len(candidate_serials) > 1:
      raise AmbiguousDriverIdentifier(
          "Found more than one adb devices matching "
          f"'{value}': {candidate_serials}")
    if len(candidate_serials) == 0:
      logging.debug("No matching adb devices found.")
      return None
    assert len(candidate_serials) == 1
    return DriverConfig(
        BrowserDriverType.ANDROID, device_id=candidate_serials[0])

  @classmethod
  def try_parse_ios_settings(cls, value: str,
                             platform: plt.Platform) -> Optional[DriverConfig]:
    candidate_serials: List[str] = []
    pattern: re.Pattern = cls.compile_search_pattern(value)
    for uuid, device_info in ios_devices(platform).items():
      if pattern.fullmatch(uuid):
        candidate_serials.append(uuid)
        continue
      if pattern.fullmatch(device_info.name):
        candidate_serials.append(uuid)
        continue
    if len(candidate_serials) > 1:
      raise AmbiguousDriverIdentifier(
          "Found more than one ios devices matching "
          f"'{value}': {candidate_serials}")
    if len(candidate_serials) == 0:
      logging.debug("No matching ios devices found.")
      return None
    assert len(candidate_serials) == 1
    return DriverConfig(BrowserDriverType.IOS, device_id=candidate_serials[0])

  @classmethod
  def compile_search_pattern(cls, maybe_pattern: str) -> re.Pattern:
    try:
      return re.compile(maybe_pattern)
    except Exception as e:  # pylint: disable=broad-except
      logging.debug(
          "Falling back to full string match for "
          "invalid regexp search pattern: %s %s", maybe_pattern, e)
      return re.compile(re.escape(maybe_pattern))

  @classmethod
  def parse_dict(cls, config: Dict[str, Any]) -> DriverConfig:
    return cls.config_parser().parse(config)

  @classmethod
  def config_parser(cls) -> ConfigParser[DriverConfig]:
    parser = ConfigParser(cls)
    parser.add_argument(
        "type",
        type=BrowserDriverType.parse,
        default=BrowserDriverType.default())
    parser.add_argument(
        "path",
        type=driver_path,
        depends_on=("type",),
        help="Path to the driver executable")
    parser.add_argument(
        "settings",
        type=immutabledict,
        help="Additional driver-dependent settings.")
    parser.add_argument(
        "device_id",
        type=driver_device_id,
        depends_on=("settings",),
        help="Device ID / Serial ID / Unique device name")
    parser.add_argument(
        "adb_bin",
        type=PathParser.binary_path,
        help="Path to the adb binary, only valid for Android.")
    return parser

  def __post_init__(self):
    if not self.type:
      raise ValueError(f"{type(self).__name__}.type cannot be None.")
    try:
      hash(self.settings)
    except ValueError as e:
      raise ValueError(
          f"settings must be hashable but got: {self.settings}") from e
    self.validate()

  @property
  def is_remote(self) -> bool:
    return self.type.is_remote_driver

  @property
  def is_local(self) -> bool:
    return self.type.is_local_driver

  def validate(self) -> None:
    if self.type == BrowserDriverType.ANDROID:
      self.validate_android()
    elif self.adb_bin:
      raise argparse.ArgumentTypeError("adb_path is only valid for Android.")
    if self.type == BrowserDriverType.IOS:
      self.validate_ios()
    if self.type == BrowserDriverType.CHROMEOS_SSH:
      # Unlike the validation functions above for iOS and Android,
      # which validate the "host" to which the device is connected,
      # the ChromeOS validation function validates the "client".
      # Consider moving this logic elsewhere in the future.
      self.validate_chromeos()
    if self.is_local:
      self.validate_local()

  def validate_local(self) -> None:
    if self.path:
      PathParser.binary_path(self.path)

  def validate_android(self) -> None:
    platform = plt.PLATFORM
    devices = adb_devices(platform, self.adb_bin)
    names = list(devices.keys())
    if not devices:
      raise argparse.ArgumentTypeError("No ADB devices attached.")
    if not self.device_id:
      if len(devices) == 1:
        # Default device "adb" (no settings) with exactly one device is ok.
        return
      raise AmbiguousDriverIdentifier(
          f"{len(devices)} ADB devices connected: {names}. "
          "Please explicitly specify a device ID.")
    if self.device_id not in devices:
      raise argparse.ArgumentTypeError(
          f"Could not find ADB device with device_id={repr(self.device_id)}. "
          f"Choices are {names}.")
    if self.adb_bin:
      PathParser.binary_path(self.adb_bin, platform=platform)

  def validate_chromeos(self) -> None:
    platform = self.get_platform()
    assert isinstance(platform, ChromeOsSshPlatform), \
           f"Invalid platform: {platform}"
    platform = cast(ChromeOsSshPlatform, platform)
    if not platform.exists(platform.AUTOLOGIN_PATH):
      raise ValueError(f"Could not find `autotest` on {platform.host}."
                       "Please ensure that it is running a test image:"
                       "go/arc-setup-dev-mode-dut#usb-cros-test-image")

  def validate_ios(self) -> None:
    devices: Dict[str, Any] = ios_devices(plt.PLATFORM)
    if not devices:
      raise argparse.ArgumentTypeError("No iOS devices attached.")
    names = list(map(str, devices))
    if not self.device_id:
      if len(devices) == 1:
        # Default device "ios" (no settings) with exactly one device is ok.
        return
      raise AmbiguousDriverIdentifier(
          f"{len(devices)} ios devices connected: {names}. "
          "Please explicitly specify a device UUID.")
    if self.device_id not in devices:
      raise argparse.ArgumentTypeError(
          f"Could not find ios device with device_id={repr(self.device_id)}. "
          f"Choices are {names}.")

  def get_platform(self) -> plt.Platform:
    if self.type == BrowserDriverType.ANDROID:
      return self.get_adb_platform()
    if self.type == BrowserDriverType.IOS:
      # TODO(cbruni): use `xcrun xctrace list devices` to find the UDID
      # for attached simulators or devices. Currently only a single device
      # is supported
      pass
    if self.type in (BrowserDriverType.LINUX_SSH,
                     BrowserDriverType.CHROMEOS_SSH):
      return self.get_ssh_platform()
    return plt.PLATFORM

  def _parse_ssh_platform_driver_port(self) -> int:
    port = None
    if settings := self.settings:
      port = settings.get("port")
    if port in (None, 0):
      # The driver port is allowed to be 0 on ssh platforms. If so, we will
      # automatically start chromedriver.
      return 0
    return NumberParser.port_number(port)

  def get_ssh_platform(self) -> plt.Platform:
    assert self.settings
    host = ObjectParser.non_empty_str(self.settings.get("host"), "host")
    port = self._parse_ssh_platform_driver_port()
    ssh_port = NumberParser.port_number(
        self.settings.get("ssh_port"), "ssh port")
    ssh_user = ObjectParser.non_empty_str(
        self.settings.get("ssh_user"), "ssh user")
    if self.type == BrowserDriverType.CHROMEOS_SSH:
      return ChromeOsSshPlatform(
          plt.PLATFORM,
          host=host,
          port=port,
          ssh_port=ssh_port,
          ssh_user=ssh_user)
    return plt.LinuxSshPlatform(
        plt.PLATFORM,
        host=host,
        port=port,
        ssh_port=ssh_port,
        ssh_user=ssh_user)

  def get_adb_platform(self) -> plt.Platform:
    adb = Adb(plt.PLATFORM, self.device_id, self.adb_bin)
    return AndroidAdbPlatform(plt.PLATFORM, self.device_id, adb)

def driver_device_id(device_id: Optional[str],
                     settings: Optional[immutabledict]) -> Optional[str]:
  if not settings:
    return device_id
  settings_device_id = settings.get("device_id")
  if not device_id:
    return settings_device_id
  if settings_device_id != device_id:
    raise TypeError("Conflicting both driver['settings']['device_id'] "
                    "and driver['device_id']: "
                    f"{repr(settings_device_id)} vs {repr(device_id)}")
  return device_id
