# Copyright 2023 The Chromium Authors
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

from __future__ import annotations

import argparse
import dataclasses
import enum
from typing import Any, Callable, Dict, List, Optional, Union

from crossbench import compat
from crossbench.config import ConfigObject
from crossbench.parse import ObjectParser


@enum.unique
class ValidationMode(compat.StrEnumWithHelp):
  THROW = ("throw", "Strict mode, throw and abort on env issues")
  PROMPT = ("prompt", "Prompt to accept potential env issues")
  WARN = ("warn", "Only display a warning for env issue")
  SKIP = ("skip", "Don't perform any env validation")


def merge_bool(name: str, left: Optional[bool],
               right: Optional[bool]) -> Optional[bool]:
  if left is None:
    return right
  if right is None:
    return left
  if left != right:
    raise ValueError(f"Conflicting merge values for {name}: "
                     f"{left} vs. {right}")
  return left


Number = Union[float, int]


def merge_number_max(name: str, left: Optional[Number],
                     right: Optional[Number]) -> Optional[Number]:
  del name
  if left is None:
    return right
  if right is None:
    return left
  return max(left, right)


def merge_number_min(name: str, left: Optional[Number],
                     right: Optional[Number]) -> Optional[Number]:
  del name
  if left is None:
    return right
  if right is None:
    return left
  return min(left, right)


def merge_str_list(name: str, left: Optional[List[str]],
                   right: Optional[List[str]]) -> Optional[List[str]]:
  del name
  if left is None:
    return right
  if right is None:
    return left
  return left + right


ENV_CONFIG_PRESETS: Dict[str, "EnvironmentConfig"] = {}


@dataclasses.dataclass(frozen=True)
class EnvironmentConfig(ConfigObject):
  IGNORE = None

  disk_min_free_space_gib: Optional[float] = IGNORE
  power_use_battery: Optional[bool] = IGNORE
  screen_brightness_percent: Optional[int] = IGNORE
  cpu_max_usage_percent: Optional[float] = IGNORE
  cpu_min_relative_speed: Optional[float] = IGNORE
  system_allow_monitoring: Optional[bool] = IGNORE
  browser_allow_existing_process: Optional[bool] = IGNORE
  browser_allow_background: Optional[bool] = IGNORE
  browser_is_headless: Optional[bool] = IGNORE
  require_probes: Optional[bool] = IGNORE
  system_forbidden_process_names: Optional[List[str]] = IGNORE
  screen_allow_autobrightness: Optional[bool] = IGNORE

  @classmethod
  def default(cls) -> EnvironmentConfig:
    return ENV_CONFIG_PRESETS["default"]

  @classmethod
  def parse_str(cls, value: str) -> EnvironmentConfig:
    value = ObjectParser.non_empty_str(value)
    if preset := ENV_CONFIG_PRESETS.get(value):
      return preset
    if value[0] == "{":
      return cls.parse_inline_hjson(value)
    raise argparse.ArgumentTypeError(
        f"Unknown host config preset {repr(value)}. "
        f"Choices are {','.join(ENV_CONFIG_PRESETS.keys())}")

  @classmethod
  def parse_dict(cls, config: Dict[str, Any]) -> EnvironmentConfig:
    if "env" in config:
      config = config["env"]
    return cls(**config)

  def merge(self, other: EnvironmentConfig) -> EnvironmentConfig:
    mergers: Dict[str, Callable[[str, Any, Any], Any]] = {
        "disk_min_free_space_gib": merge_number_max,
        "power_use_battery": merge_bool,
        "screen_brightness_percent": merge_number_max,
        "cpu_max_usage_percent": merge_number_min,
        "cpu_min_relative_speed": merge_number_max,
        "system_allow_monitoring": merge_bool,
        "browser_allow_existing_process": merge_bool,
        "browser_allow_background": merge_bool,
        "browser_is_headless": merge_bool,
        "require_probes": merge_bool,
        "system_forbidden_process_names": merge_str_list,
        "screen_allow_autobrightness": merge_bool,
    }
    kwargs = {}
    for name, merger in mergers.items():
      self_value = getattr(self, name)
      other_value = getattr(other, name)
      kwargs[name] = merger(name, self_value, other_value)
    return EnvironmentConfig(**kwargs)


_config_default = EnvironmentConfig()
_config_strict = EnvironmentConfig(
    cpu_max_usage_percent=98,
    cpu_min_relative_speed=1,
    system_allow_monitoring=False,
    browser_allow_existing_process=False,
    require_probes=True,
)
_config_battery = _config_strict.merge(
    EnvironmentConfig(power_use_battery=True))
_config_power = _config_strict.merge(EnvironmentConfig(power_use_battery=False))
_config_catan = _config_strict.merge(
    EnvironmentConfig(
        screen_brightness_percent=65,
        system_forbidden_process_names=["terminal", "iterm2"],
        screen_allow_autobrightness=False))

ENV_CONFIG_PRESETS.update({
    "default": _config_default,
    "strict": _config_strict,
    "battery": _config_battery,
    "power": _config_power,
    "catan": _config_catan,
})
