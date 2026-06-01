# Copyright 2024 The Chromium Authors
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

from __future__ import annotations

import datetime as dt
from typing import TYPE_CHECKING, Tuple, Type

from crossbench.action_runner.action.action import ACTION_TIMEOUT, ActionT
from crossbench.action_runner.action.action_type import ActionType
from crossbench.action_runner.action.base_input_source import InputSourceAction
from crossbench.benchmarks.loading.input_source import InputSource
from crossbench.parse import DurationParser, ObjectParser

if TYPE_CHECKING:
  from crossbench.action_runner.base import ActionRunner
  from crossbench.config import ConfigParser
  from crossbench.runner.run import Run
  from crossbench.types import JsonDict


class TextInputAction(InputSourceAction):
  TYPE: ActionType = ActionType.TEXT_INPUT

  @classmethod
  def config_parser(cls: Type[ActionT]) -> ConfigParser[ActionT]:
    parser = super().config_parser()
    parser.add_argument("text", type=ObjectParser.non_empty_str, required=True)
    parser.add_argument(
        "duration",
        type=DurationParser.positive_or_zero_duration,
        default=dt.timedelta())
    return parser

  def __init__(self,
               source: InputSource,
               duration: dt.timedelta,
               text: str,
               timeout: dt.timedelta = ACTION_TIMEOUT,
               index: int = 0) -> None:
    self._text: str = text
    super().__init__(source, duration, timeout, index)

  @property
  def text(self) -> str:
    return self._text

  def run_with(self, run: Run, action_runner: ActionRunner) -> None:
    action_runner.text_input(run, self)

  def validate(self) -> None:
    super().validate()
    if not self._text:
      raise ValueError(f"{self}.text is missing.")

  def validate_duration(self) -> None:
    # A text input action is allowed to have a zero duration.
    return

  def supported_input_sources(self) -> Tuple[InputSource, ...]:
    return (InputSource.JS, InputSource.KEYBOARD)

  def to_json(self) -> JsonDict:
    details = super().to_json()
    details["text"] = self._text
    return details
