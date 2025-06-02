# Copyright 2023 The Chromium Authors
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

from __future__ import annotations
import argparse

import dataclasses
import enum
import pathlib
from typing import Any, Dict, Optional

from crossbench import cli_helper, compat
from crossbench.config import ConfigObject, ConfigParser
from crossbench.network import ts_proxy


@enum.unique
class NetworkType(compat.StrEnumWithHelp):
  LIVE = ("live", "Live network.")
  REPLAY = ("replay", "Replayed network from a wpr.go archive.")
  LOCAL = ("local", "Serve content from a local http file server.")


def _settings_str(name: str) -> str:
  settings = ts_proxy.TRAFFIC_SETTINGS[name]
  return (f"rtt={settings['rtt_ms']}ms, "
          f"in={settings['in_kbps']} kbps,"
          f"out={settings['out_kbps']} kbps")


@enum.unique
class NetworkSpeedPreset(compat.StrEnumWithHelp):
  """Presets that match ts_proxy settings."""
  LIVE = ("live", "Untroubled default network settings")
  MOBILE_3G_SLOW = ("3G-slow",
                    f"Slow 3G network settings: {_settings_str('3G-slow')}")
  MOBILE_3G_REGULAR = (
      "3G-regular",
      f"Regular 3G network settings: {_settings_str('3G-regular')}")
  MOBILE_3G_FAST = ("3G-fast",
                    f"Slow 3G network settings: {_settings_str('3G-fast')}")
  MOBILE_4G = ("4G", f"Regular 4G network settings: {_settings_str('4G')}")


@dataclasses.dataclass(frozen=True)
class NetworkSpeedConfig(ConfigObject):

  @classmethod
  def default(cls) -> NetworkSpeedConfig:
    return NetworkSpeedConfig()

  @classmethod
  def loads(cls, value: str) -> NetworkSpeedConfig:
    if not value:
      raise argparse.ArgumentTypeError("Cannot parse empty string")
    # TODO: implement
    return cls.default()

  @classmethod
  def load_dict(cls, config: Dict[str, Any]) -> NetworkSpeedConfig:
    # TODO: implement
    # return cls.config_parser().parse(config)
    return cls.default()


@dataclasses.dataclass(frozen=True)
class NetworkConfig(ConfigObject):
  type: NetworkType = NetworkType.LIVE
  speed: NetworkSpeedConfig = NetworkSpeedConfig.default()
  path: Optional[pathlib.Path] = None

  @classmethod
  def default(cls) -> NetworkConfig:
    return NetworkConfig()

  @classmethod
  def config_parser(cls) -> ConfigParser[NetworkConfig]:
    parser = ConfigParser("DriverConfig parser", cls)
    parser.add_argument("type", type=NetworkType, default=NetworkType.LIVE)
    parser.add_argument("speed", type=NetworkSpeedConfig)
    parser.add_argument(
        "path", type=cli_helper.parse_existing_file_path, required=False)
    return parser

  @classmethod
  def help(cls) -> str:
    return cls.config_parser().help

  @classmethod
  def loads(cls, value: str) -> NetworkConfig:
    # TODO: implement
    if not value:
      raise argparse.ArgumentTypeError("Cannot parse empty string")
    return cls.default()

  @classmethod
  def is_valid_path(cls, path: pathlib.Path) -> bool:
    if path.suffix == ".archive":
      return True
    return super().is_valid_path(path)

  @classmethod
  def load_path(cls, path: pathlib.Path) -> NetworkConfig:
    if path.suffix == ".archive":
      return cls._load_wpr_archive(path)
    return super().load_path(path)

  @classmethod
  def _load_wpr_archive(cls, path: pathlib.Path) -> NetworkConfig:
    path = cli_helper.parse_non_empty_file_path(path, "wpr.go archive")
    return NetworkConfig(type=NetworkType.REPLAY, path=path)

  @classmethod
  def load_dict(cls, config: Dict[str, Any]) -> NetworkConfig:
    return cls.config_parser().parse(config)

  def validate(self) -> None:
    if not self.type:
      raise argparse.ArgumentTypeError("Missing NetworkConfig.type.")
    if self.type == NetworkType.LIVE:
      if self.path:
        raise argparse.ArgumentTypeError(
            "NetworkConfig path cannot be used with type=live")
    elif self.type == NetworkType.REPLAY:
      if not self.path:
        raise argparse.ArgumentTypeError(
            "NetworkConfig with type=replay requires "
            "a valid wpr.go archive path.")
      cli_helper.parse_non_empty_file_path(self.path, "wpr.go-archive")
    elif self.type == NetworkType.LOCAL:
      if not self.path:
        raise argparse.ArgumentTypeError(
            "NetworkConfig with type=local requires "
            "a valid local dir path to serve files.")
      cli_helper.parse_non_empty_dir_path(self.path, "local-serve dir")
