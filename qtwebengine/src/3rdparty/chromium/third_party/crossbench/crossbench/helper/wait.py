# Copyright 2024 The Chromium Authors
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

from __future__ import annotations

import datetime as dt
from typing import Iterator, Optional, Tuple, Union

from crossbench import plt


def as_timedelta(value: Union[int, float, dt.timedelta]) -> dt.timedelta:
  if isinstance(value, dt.timedelta):
    return value
  return dt.timedelta(seconds=value)


class WaitRange:
  """
  Create wait/sleep ranges with the given parameters:

  If present we start with the initial delay, and then exponentially
  increase the sleep/wait time by the given factor, until we reach the max
  sleep time.

  | delay | min | min * factor | ... | min * factor ** N | max | ... | max |
  | ----------------------------- timeout ---------------------------------|

  The timeout puts an upper bound to the total sleep time when using
  wait_with_backoff().
  """
  min: dt.timedelta
  max: dt.timedelta
  initial_sleep: dt.timedelta
  max_iterations: Optional[int]

  def __init__(
      self,
      min: Union[int, float, dt.timedelta] = 0.1,  # pylint: disable=redefined-builtin
      timeout: Union[int, float, dt.timedelta] = 10,
      factor: float = 1.01,
      max: Optional[Union[int, float, dt.timedelta]] = None,  # pylint: disable=redefined-builtin
      max_iterations: Optional[int] = None,
      delay: Union[int, float, dt.timedelta] = 0) -> None:
    self.min = as_timedelta(min)
    assert self.min.total_seconds() > 0
    if not max:
      self.max = self.min * 10
    else:
      self.max = as_timedelta(max)
    assert self.min <= self.max
    assert 1.0 < factor
    self.factor = factor
    self.timeout = as_timedelta(timeout)
    assert 0 < self.timeout.total_seconds()
    self.delay = as_timedelta(delay)
    assert self.delay <= self.timeout
    assert max_iterations is None or max_iterations > 0
    self.max_iterations = max_iterations

  def __iter__(self) -> Iterator[dt.timedelta]:
    i = 0
    if self.delay:
      yield self.delay
    current_sleep = self.min
    while self.max_iterations is None or i < self.max_iterations:
      yield current_sleep
      current_sleep = min(current_sleep * self.factor, self.max)
      i += 1

  def wait_with_backoff(
      self,
      platform: Optional[plt.Platform] = None) -> Iterator[Tuple[float, float]]:
    platform = platform or plt.PLATFORM
    start = dt.datetime.now()
    timeout = self.timeout
    for sleep_for in self:
      duration = dt.datetime.now() - start
      if duration > self.timeout:
        raise TimeoutError(f"Waited for {duration}")
      time_left = timeout - duration
      yield duration.total_seconds(), time_left.total_seconds()
      platform.sleep(sleep_for.total_seconds())


def wait_with_backoff(
    wait_range: Union[int, float, dt.timedelta, WaitRange],
    platform: Optional[plt.Platform] = None) -> Iterator[Tuple[float, float]]:
  if not isinstance(wait_range, WaitRange):
    wait_range = WaitRange(timeout=wait_range)
  return wait_range.wait_with_backoff(platform)
