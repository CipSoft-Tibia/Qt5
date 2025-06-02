# Copyright 2023 The Chromium Authors
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

from __future__ import annotations
import abc

import datetime as dt
import time
import logging
import pathlib
import threading
from typing import TYPE_CHECKING, Iterable, Sequence, Tuple
from crossbench import cli_helper

from crossbench.probes import probe as cb_probe
from crossbench.probes.results import LocalProbeResult, ProbeResult

if TYPE_CHECKING:
  from crossbench.env import HostEnvironment
  from crossbench import plt
  from crossbench.runner.run import Run


class PollingProbe(cb_probe.Probe, abc.ABC):
  """
  Abstract probe to periodically collect the results of any bash cmd.
  """
  NAME = "polling"
  IS_GENERAL_PURPOSE = False

  @classmethod
  def config_parser(cls) -> cb_probe.ProbeConfigParser:
    parser = super().config_parser()
    parser.add_argument(
        "interval",
        type=cli_helper.Duration.parse_non_zero,
        default=dt.timedelta(seconds=1),
        help="Run the cmd at this interval and produce separate results.")
    return parser

  def __init__(
      self,
      cmd: Iterable[str],
      interval: dt.timedelta = dt.timedelta(seconds=1)) -> None:
    super().__init__()
    self._cmd = tuple(cmd)
    self._interval = interval
    if interval.total_seconds() < 0.1:
      raise ValueError(f"Polling interval must be >= 0.1s, but got: {interval}")

  @property
  def key(self) -> Tuple[Tuple, ...]:
    return super().key + (("cmd", tuple(self.cmd)),
                          ("interval", self.interval.total_seconds()))

  @property
  def interval(self) -> dt.timedelta:
    return self._interval

  @property
  def cmd(self) -> Tuple[str]:
    return self._cmd

  def validate_env(self, env: HostEnvironment) -> None:
    super().validate_env(env)
    if env.runner.repetitions != 1:
      env.handle_warning(f"Probe={self.NAME} cannot merge data over multiple "
                         f"repetitions={env.runner.repetitions}.")

  def get_context(self, run: Run) -> PollingProbeContext:
    return PollingProbeContext(self, run)


class ShellPollingProbe(PollingProbe):
  """
  General-purpose probe to periodically collect the stdout of a given bash cmd.
  """

  IS_GENERAL_PURPOSE = True
  NAME = "poll"

  @classmethod
  def config_parser(cls) -> cb_probe.ProbeConfigParser:
    parser = super().config_parser()
    parser.add_argument(
        "cmd",
        type=cli_helper.parse_sh_cmd,
        required=True,
        help="Write stdout of this CMD as a result.")
    return parser


class PollingProbeContext(cb_probe.ProbeContext[PollingProbe]):
  _poller: CMDPoller

  def __init__(self, probe: PollingProbe, run: Run) -> None:
    super().__init__(probe, run)
    self._poller = CMDPoller(self.browser_platform, self.probe.cmd,
                             self.probe.interval, self.result_path)

  def setup(self) -> None:
    self.result_path.mkdir()

  def start(self) -> None:
    self._poller.start()

  def stop(self) -> None:
    self._poller.stop()

  def tear_down(self) -> ProbeResult:
    return LocalProbeResult(file=(self.result_path,))


class CMDPoller(threading.Thread):

  def __init__(self, platform: plt.Platform, cmd: Tuple[str],
               interval: dt.timedelta, path: pathlib.Path):
    super().__init__()
    self._platform = platform
    self._cmd = cmd
    self._path = path
    if interval < dt.timedelta(seconds=0.1):
      raise ValueError("Poller interval should be >= 0.1s for accuracy, "
                       f"but got {interval}s")
    self._interval_seconds = interval.total_seconds()
    self._event = threading.Event()

  def stop(self) -> None:
    self._event.set()
    self.join()

  def run(self) -> None:
    start_time = time.monotonic_ns()
    while not self._event.is_set():
      poll_start = dt.datetime.now()

      data = self._platform.sh_stdout(*self._cmd)
      datetime_str = poll_start.strftime("%Y-%m-%d_%H%M%S_%f")
      out_file = self._path / f"{datetime_str}.txt"
      with out_file.open("w", encoding="utf-8") as f:
        f.write(data)

      poll_end = dt.datetime.now()
      diff = (poll_end - poll_start).total_seconds()
      if diff > self._interval_seconds:
        logging.warning("Poller command took longer than expected %fs: %s",
                        self._interval_seconds, self._cmd)

      # Calculate wait_time against fixed start time to avoid drifting.
      total_time = ((time.monotonic_ns() - start_time) / 10.0**9)
      wait_time = self._interval_seconds - (total_time % self._interval_seconds)
      self._event.wait(wait_time)
