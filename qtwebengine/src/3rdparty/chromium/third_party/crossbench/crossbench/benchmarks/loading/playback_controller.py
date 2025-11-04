# Copyright 2023 The Chromium Authors
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

from __future__ import annotations

import abc
import argparse
import dataclasses
import datetime as dt
from typing import Iterator

from crossbench import cli_helper


class PlaybackController(abc.ABC):

  @classmethod
  def parse(cls, value: str) -> PlaybackController:
    if not value or value == "once":
      return cls.once()
    if value in ("inf", "infinity", "forever"):
      return cls.forever()
    if value[-1].isnumeric():
      raise argparse.ArgumentTypeError(
          f"Missing unit suffix: '{value}'\n"
          "Use 'x' for repetitions or time unit 's', 'm', 'h'")
    if value[-1] == "x":
      loops = cli_helper.parse_positive_int(value[:-1], "Repeat-count")
      return cls.repeat(loops)
    duration = cli_helper.Duration.parse_non_zero(value)
    return cls.timeout(duration)

  @classmethod
  def default(cls):
    return cls.once()

  @classmethod
  def once(cls) -> RepeatPlaybackController:
    return RepeatPlaybackController(1)

  @classmethod
  def repeat(cls, count: int) -> RepeatPlaybackController:
    return RepeatPlaybackController(count)

  @classmethod
  def forever(cls) -> PlaybackController:
    return ForeverPlaybackController()

  @classmethod
  def timeout(cls, duration: dt.timedelta) -> TimeoutPlaybackController:
    return TimeoutPlaybackController(duration)

  @abc.abstractmethod
  def __iter__(self) -> Iterator[None]:
    pass


@dataclasses.dataclass(frozen=True)
class ForeverPlaybackController(PlaybackController):

  def __iter__(self) -> Iterator[None]:
    while True:
      yield None


@dataclasses.dataclass(frozen=True)
class TimeoutPlaybackController(PlaybackController):
  duration : dt.timedelta

  def __iter__(self) -> Iterator[None]:
    end = dt.datetime.now() + self.duration
    while True:
      yield None
      if dt.datetime.now() > end:
        return


@dataclasses.dataclass(frozen=True)
class RepeatPlaybackController(PlaybackController):
  count : int

  def __post_init__(self):
    cli_helper.parse_positive_int(self.count, " page playback count")

  def __iter__(self) -> Iterator[None]:
    for _ in range(self.count):
      yield None