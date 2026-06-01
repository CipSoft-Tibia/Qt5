# Copyright 2024 The Chromium Authors
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

from __future__ import annotations

import datetime as dt
from typing import TYPE_CHECKING, Optional, Tuple, Type

from crossbench.action_runner.action.action import ACTION_TIMEOUT, ActionT
from crossbench.action_runner.action.action_type import ActionType
from crossbench.action_runner.action.base_input_source import InputSourceAction
from crossbench.action_runner.action.position import PositionConfig
from crossbench.benchmarks.loading.input_source import InputSource
from crossbench.parse import DurationParser, ObjectParser

if TYPE_CHECKING:
  from crossbench.action_runner.base import ActionRunner
  from crossbench.config import ConfigParser
  from crossbench.runner.run import Run
  from crossbench.types import JsonDict


class ClickAction(InputSourceAction):
  TYPE: ActionType = ActionType.CLICK

  @classmethod
  def config_parser(cls: Type[ActionT]) -> ConfigParser[ActionT]:
    parser = super().config_parser()
    parser.add_argument(
        "position",
        aliases=("pos", "selector"),
        type=PositionConfig,
        required=True)
    parser.add_argument(
        "duration",
        type=DurationParser.positive_or_zero_duration,
        default=dt.timedelta())
    parser.add_argument("verify", type=ObjectParser.non_empty_str)
    return parser

  def __init__(self,
               source: InputSource,
               position: PositionConfig,
               duration: dt.timedelta = dt.timedelta(),
               verify: Optional[str] = None,
               timeout: dt.timedelta = ACTION_TIMEOUT,
               index: int = 0):
    self._position = position
    self._verify = verify
    super().__init__(source, duration, timeout, index)

  @property
  def position(self) -> PositionConfig:
    return self._position

  @property
  def verify(self) -> Optional[str]:
    return self._verify

  def run_with(self, run: Run, action_runner: ActionRunner) -> None:
    action_runner.click(run, self)

  def validate(self) -> None:
    super().validate()

    if self._input_source is InputSource.JS and self.position.coordinates:
      raise ValueError("X,Y Coordinates cannot be used with JS click source.")

  def validate_duration(self) -> None:
    # A click action is allowed to have a zero duration.
    return

  def supported_input_sources(self) -> Tuple[InputSource, ...]:
    return (InputSource.JS, InputSource.TOUCH, InputSource.MOUSE)

  def to_json(self) -> JsonDict:
    details = super().to_json()
    details["position"] = self._position.to_json()
    if self._verify:
      details["verify"] = self._verify
    return details
