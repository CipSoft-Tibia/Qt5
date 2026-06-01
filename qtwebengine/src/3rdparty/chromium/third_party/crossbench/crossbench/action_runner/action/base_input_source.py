# Copyright 2024 The Chromium Authors
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

from __future__ import annotations

import abc
import datetime as dt
from typing import TYPE_CHECKING, Tuple, Type

from crossbench.action_runner.action.action import ACTION_TIMEOUT, ActionT
from crossbench.action_runner.action.base_duration import BaseDurationAction
from crossbench.benchmarks.loading.input_source import InputSource

if TYPE_CHECKING:
  from crossbench.config import ConfigParser
  from crossbench.types import JsonDict


class InputSourceAction(BaseDurationAction, metaclass=abc.ABCMeta):

  @classmethod
  def config_parser(cls: Type[ActionT]) -> ConfigParser[ActionT]:
    parser = super().config_parser()
    parser.add_argument(
        "source", type=InputSource.parse, default=InputSource.JS)
    return parser

  def __init__(self,
               source: InputSource,
               duration: dt.timedelta,
               timeout: dt.timedelta = ACTION_TIMEOUT,
               index: int = 0) -> None:
    self._input_source = source
    super().__init__(duration, timeout, index)

  @property
  def input_source(self) -> InputSource:
    return self._input_source

  def validate(self) -> None:
    super().validate()
    self.validate_input_source()

  def validate_input_source(self) -> None:
    if self.input_source not in self.supported_input_sources():
      raise ValueError(
          f"Unsupported input source for {self.__class__.__name__}")

  @abc.abstractmethod
  def supported_input_sources(self) -> Tuple[InputSource, ...]:
    pass

  def to_json(self) -> JsonDict:
    details = super().to_json()
    details["source"] = self.input_source
    return details
