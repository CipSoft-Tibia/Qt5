# Copyright 2024 The Chromium Authors
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

from __future__ import annotations

import datetime as dt
from typing import TYPE_CHECKING, Type

from crossbench.action_runner.action.action import (ACTION_TIMEOUT, Action,
                                                    ActionT)
from crossbench.action_runner.action.action_type import ActionType
from crossbench.parse import ObjectParser

if TYPE_CHECKING:
  from crossbench.action_runner.base import ActionRunner
  from crossbench.config import ConfigParser
  from crossbench.runner.run import Run
  from crossbench.types import JsonDict


class WaitForElementAction(Action):
  TYPE: ActionType = ActionType.WAIT_FOR_ELEMENT

  @classmethod
  def config_parser(cls: Type[ActionT]) -> ConfigParser[ActionT]:
    parser = super().config_parser()
    parser.add_argument(
        "selector", type=ObjectParser.non_empty_str, required=True)
    return parser

  def __init__(self,
               selector: str,
               timeout: dt.timedelta = ACTION_TIMEOUT,
               index: int = 0):
    self._selector = selector
    super().__init__(timeout, index)

  @property
  def selector(self) -> str:
    return self._selector

  def run_with(self, run: Run, action_runner: ActionRunner) -> None:
    action_runner.wait_for_element(run, self)

  def validate(self) -> None:
    super().validate()
    if not self.selector:
      raise ValueError(f"{self}.selector is missing.")

  def to_json(self) -> JsonDict:
    details = super().to_json()
    details["selector"] = self.selector
    return details
