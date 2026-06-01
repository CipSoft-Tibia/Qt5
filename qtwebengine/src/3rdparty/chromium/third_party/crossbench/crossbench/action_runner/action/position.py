# Copyright 2024 The Chromium Authors
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

from __future__ import annotations

import argparse
import dataclasses
from typing import TYPE_CHECKING, Dict, Optional

from crossbench.benchmarks.loading.point import Point
from crossbench.config import ConfigObject, ConfigParser, UnusedPropertiesMode
from crossbench.parse import NumberParser, ObjectParser

if TYPE_CHECKING:
  from crossbench.types import JsonDict


@dataclasses.dataclass(frozen=True)
class CoordinatesConfig(ConfigObject):
  x: int
  y: int

  @classmethod
  def parse_dict(cls, config: Dict) -> CoordinatesConfig:
    return cls.config_parser().parse(config)

  @classmethod
  def parse_str(cls, value):
    del value
    raise NotImplementedError("Cannot create CoordinatesConfig from string")

  @classmethod
  def config_parser(cls) -> ConfigParser[CoordinatesConfig]:
    parser = ConfigParser(
        cls, unused_properties_mode=UnusedPropertiesMode.ERROR)
    parser.add_argument("x", type=NumberParser.positive_zero_int, required=True)
    parser.add_argument("y", type=NumberParser.positive_zero_int, required=True)
    return parser

  def point(self) -> Point:
    return Point(self.x, self.y)


@dataclasses.dataclass(frozen=True)
class SelectorConfig(ConfigObject):
  selector: str

  required: bool
  scroll_into_view: bool
  wait: bool

  @classmethod
  def parse_str(cls, value) -> SelectorConfig:
    selector = ObjectParser.non_empty_str(value, "selector")
    return cls(
        selector=selector, required=True, scroll_into_view=False, wait=False)

  @classmethod
  def parse_dict(cls, config: Dict) -> SelectorConfig:
    return cls.config_parser().parse(config)

  @classmethod
  def config_parser(cls) -> ConfigParser[SelectorConfig]:
    parser = ConfigParser(
        cls, unused_properties_mode=UnusedPropertiesMode.ERROR)
    parser.add_argument(
        "selector", type=ObjectParser.non_empty_str, required=True)
    parser.add_argument("required", type=ObjectParser.bool, default=True)
    parser.add_argument(
        "scroll_into_view", type=ObjectParser.bool, default=False)
    parser.add_argument("wait", type=ObjectParser.bool, default=False)
    return parser


@dataclasses.dataclass(frozen=True)
class PositionConfig(ConfigObject):
  coordinates: Optional[CoordinatesConfig] = None
  selector: Optional[SelectorConfig] = None

  @classmethod
  def parse_str(cls, value) -> PositionConfig:
    return cls(selector=SelectorConfig.parse_str(value))

  @classmethod
  def parse_dict(cls, config: Dict) -> PositionConfig:
    selector_parser = SelectorConfig.config_parser()
    if selector_parser.has_all_required_args(config):
      return cls(selector=selector_parser.parse(config))

    coordinates_parser = CoordinatesConfig.config_parser()
    if coordinates_parser.has_all_required_args(config):
      return cls(coordinates=coordinates_parser.parse(config))

    raise argparse.ArgumentTypeError(
        f"{config} is not a valid coordinate or selector")

  @classmethod
  def from_coordinates(cls, x: int, y: int) -> PositionConfig:
    return cls(coordinates=CoordinatesConfig(x, y))

  @classmethod
  def from_selector(cls,
                    selector: str,
                    required: bool = True,
                    scroll_into_view: bool = False,
                    wait: bool = False) -> PositionConfig:
    return cls(
        selector=SelectorConfig(
            selector=selector,
            required=required,
            scroll_into_view=scroll_into_view,
            wait=wait))

  def validate(self) -> None:
    super().validate()
    if bool(self.coordinates) != bool(self.coordinates):
      raise ValueError(
          "Position config must have exactly one coordinates or selector")

  def to_json(self) -> JsonDict:
    if coordinates := self.coordinates:
      return {"x": coordinates.x, "y": coordinates.y}
    elif selector := self.selector:
      return {
          "required": selector.required,
          "scroll_into_view": selector.scroll_into_view,
          "selector": selector.selector,
          "wait": selector.wait,
      }
    raise ValueError(
        "Position config must have exactly one coordinates or selector")
