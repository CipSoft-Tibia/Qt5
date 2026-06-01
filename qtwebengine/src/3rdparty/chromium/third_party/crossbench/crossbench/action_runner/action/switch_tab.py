# Copyright 2024 The Chromium Authors
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

from __future__ import annotations

import datetime as dt
import re
from typing import TYPE_CHECKING, Optional, Type

from crossbench.action_runner.action.action import (ACTION_TIMEOUT, Action,
                                                    ActionT)
from crossbench.action_runner.action.action_type import ActionType
from crossbench.parse import NumberParser, ObjectParser

if TYPE_CHECKING:
  from crossbench.action_runner.base import ActionRunner
  from crossbench.config import ConfigParser
  from crossbench.runner.run import Run
  from crossbench.types import JsonDict


class SwitchTabAction(Action):
  TYPE: ActionType = ActionType.SWITCH_TAB

  @classmethod
  def config_parser(cls: Type[ActionT]) -> ConfigParser[ActionT]:
    parser = super().config_parser()
    parser.add_argument(
        "tab_index",
        type=NumberParser.any_int,
        help=(
            "The index of the tab to switch to. Tabs are indexed in creation "
            "order. Negative values are allowed, e.g. -1 is the most recently "
            "opened tab."))
    parser.add_argument("title", type=ObjectParser.regexp)
    parser.add_argument("url", type=ObjectParser.regexp)
    return parser

  def __init__(self,
               tab_index: Optional[int] = None,
               title: Optional[re.Pattern] = None,
               url: Optional[re.Pattern] = None,
               timeout: dt.timedelta = ACTION_TIMEOUT,
               index: int = 0) -> None:
    self._title = title
    self._url = url
    self._tab_index = tab_index
    super().__init__(timeout, index)

  @property
  def title(self) -> Optional[re.Pattern]:
    return self._title

  @property
  def url(self) -> Optional[re.Pattern]:
    return self._url

  @property
  def tab_index(self) -> Optional[int]:
    return self._tab_index

  def run_with(self, run: Run, action_runner: ActionRunner) -> None:
    action_runner.switch_tab(run, self)

  def to_json(self) -> JsonDict:
    details = super().to_json()
    if self._tab_index:
      details["tab_index"] = self._tab_index
    if self._title:
      details["title"] = str(self._title.pattern)
    if self._url:
      details["url"] = str(self._url.pattern)
    return details
