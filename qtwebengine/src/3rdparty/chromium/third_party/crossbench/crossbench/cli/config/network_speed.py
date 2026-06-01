# Copyright 2024 The Chromium Authors
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

from __future__ import annotations

import argparse
import dataclasses
import enum
from typing import TYPE_CHECKING, Any, Dict, Optional

from crossbench.config import ConfigEnum, ConfigObject, ConfigParser
from crossbench.network.traffic_shaping import ts_proxy_settings
from crossbench.parse import NumberParser, PathParser

if TYPE_CHECKING:
  from crossbench import path as pth

# We're using 'type' here a lot, let's skip the warnings from pylint.
# pylint: disable=redefined-builtin


def _settings_str(name: str) -> str:
  settings = ts_proxy_settings.TRAFFIC_SETTINGS[name]
  return (f"rtt={settings['rtt_ms']}ms, "
          f"in={settings['in_kbps']} kbps,"
          f"out={settings['out_kbps']} kbps")


@enum.unique
class NetworkSpeedPreset(ConfigEnum):
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
  ts_proxy: Optional[pth.AnyPath] = None
  rtt_ms: Optional[int] = None
  in_kbps: Optional[int] = None
  out_kbps: Optional[int] = None
  window: Optional[int] = None

  @classmethod
  def default(cls) -> NetworkSpeedConfig:
    return NetworkSpeedConfig()

  @classmethod
  def parse(cls, value: Any, **kwargs) -> NetworkSpeedConfig:
    if isinstance(value, NetworkSpeedPreset):
      return cls.parse_preset(value)
    return super().parse(value, **kwargs)

  @classmethod
  def parse_str(cls, value: str) -> NetworkSpeedConfig:
    if not value:
      raise argparse.ArgumentTypeError("Cannot parse empty string")
    if value == "default":
      return cls.default()
    preset = NetworkSpeedPreset.parse(value)
    return cls.parse_preset(preset)

  @classmethod
  def parse_preset(cls, preset: NetworkSpeedPreset) -> NetworkSpeedConfig:
    if preset == NetworkSpeedPreset.LIVE:
      return cls.default()
    preset_kwargs = ts_proxy_settings.TRAFFIC_SETTINGS[str(preset)]
    return cls(**preset_kwargs)

  @classmethod
  def parse_dict(cls, config: Dict[str, Any]) -> NetworkSpeedConfig:
    return cls.config_parser().parse(config)

  @classmethod
  def config_parser(cls) -> ConfigParser[NetworkSpeedConfig]:
    parser = ConfigParser(cls, default=NetworkSpeedConfig.default())
    parser.add_argument(
        "ts_proxy", type=PathParser.existing_file_path, required=False)
    # See tsproxy.py --help
    parser.add_argument(
        "rtt_ms",
        type=NumberParser.positive_int,
        help="Round Trip Time Latency (in ms).")
    parser.add_argument(
        "in_kbps",
        type=NumberParser.positive_int,
        help="Download Bandwidth (in 1000 bits/s - Kbps).")
    parser.add_argument(
        "out_kbps",
        type=NumberParser.positive_int,
        help="Upload Bandwidth (in 1000 bits/s - Kbps).")
    parser.add_argument(
        "window",
        default=10,
        type=NumberParser.positive_int,
        help="Emulated TCP initial congestion window (defaults to 10).")
    return parser

  @classmethod
  def help(cls) -> str:
    return cls.config_parser().help

  @property
  def is_live(self):
    return self == self.default()
