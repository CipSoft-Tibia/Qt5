# Copyright 2024 The Chromium Authors
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

from __future__ import annotations

import datetime as dt
import logging
from typing import TYPE_CHECKING, Any, Dict, Optional, Type

from crossbench import exception
from crossbench import path as pth
from crossbench.action_runner.action.action import (ACTION_TIMEOUT, Action,
                                                    ActionT)
from crossbench.action_runner.action.action_type import ActionType
from crossbench.parse import ObjectParser, PathParser

if TYPE_CHECKING:
  from crossbench.action_runner.base import ActionRunner
  from crossbench.config import ConfigParser
  from crossbench.runner.run import Run
  from crossbench.types import JsonDict


def parse_replacement_dict(value: Any) -> Dict[str, str]:
  dict_value = ObjectParser.dict(value)
  for replace_key, replace_value in dict_value.items():
    with exception.annotate_argparsing(
        f"Parsing ...[{repr(replace_key)}] = {repr(value)}"):
      ObjectParser.non_empty_str(replace_key, "replacement key")
      ObjectParser.any_str(replace_value, "replacement value")
  return dict_value


class JsAction(Action):
  TYPE: ActionType = ActionType.JS

  @classmethod
  def config_parser(cls: Type[ActionT]) -> ConfigParser[ActionT]:
    parser = super().config_parser()
    parser.add_argument("script", type=ObjectParser.non_empty_str)
    parser.add_argument(
        "script_path", aliases=("path",), type=PathParser.existing_file_path)
    parser.add_argument(
        "replacements", aliases=("replace",), type=parse_replacement_dict)
    return parser

  def __init__(self,
               script: Optional[str],
               script_path: Optional[pth.LocalPath],
               replacements: Optional[Dict[str, str]] = None,
               timeout: dt.timedelta = ACTION_TIMEOUT,
               index: int = 0) -> None:
    self._original_script = script
    self._script_path = script_path
    self._script = ""
    if bool(script) == bool(script_path):
      raise ValueError(
          f"One of {self}.script or {self}.script_path, but not both, "
          "have to specified. ")
    if script:
      self._script = script
    elif script_path:
      self._script = script_path.read_text()
      logging.debug("Loading script from %s: %s", script_path, script)
      # TODO: Support argument injection into shared file script.
    self._replacements = replacements
    if replacements:
      for key, value in replacements.items():
        self._script = self._script.replace(key, value)
    super().__init__(timeout, index)

  @property
  def script(self) -> str:
    return self._script

  def run_with(self, run: Run, action_runner: ActionRunner) -> None:
    action_runner.js(run, self)

  def validate(self) -> None:
    super().validate()
    if not self.script:
      raise ValueError(
          f"{self}.script is missing or the provided script file is empty.")

  def to_json(self) -> JsonDict:
    details = super().to_json()
    if self._original_script:
      details["script"] = self._original_script
    if self._script_path:
      details["script_path"] = str(self._script_path)
    if self._replacements:
      details["replacements"] = self._replacements
    return details
