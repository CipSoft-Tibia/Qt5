# Copyright 2023 The Chromium Authors
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

from __future__ import annotations

import atexit
import datetime as dt
import enum
import subprocess
from typing import TYPE_CHECKING, Optional, Sequence, Tuple

from crossbench import cli_helper, compat, helper
from crossbench.probes.probe import (Probe, ProbeConfigParser, ProbeContext,
                                     ResultLocation)
from crossbench.probes.results import ProbeResult

if TYPE_CHECKING:
  import pathlib

  from crossbench.browsers.browser import Browser
  from crossbench.env import HostEnvironment
  from crossbench.runner.run import Run


@enum.unique
class SamplerType(compat.StrEnumWithHelp):
  BATTERY = ("battery", "Battery level")
  CPU_POWER = ("cpu_power",
               "CPU power and per-core frequency and idle residency")
  DISK = ("disk", "Number of read/write ops/bytes")
  GPU_POWER = ("gpu_power",
               "GPU power consumption, frequency and active residency")
  INTERRUPTS = ("interrupts", "Per-core interrupt count")
  NETWORK = ("network", "Number of in/out packets/bytes")
  TASKS = ("tasks", "Per-task stats including CPU usage and wakeups")
  THERMAL = ("thermal", "Thermal pressure state")


class PowerMetricsProbe(Probe):
  """
  Probe to collect data using macOS's powermetrics command-line tool.
  """

  NAME = "powermetrics"
  RESULT_LOCATION = ResultLocation.BROWSER
  SAMPLERS: Tuple[SamplerType,
                  ...] = (SamplerType.BATTERY, SamplerType.CPU_POWER,
                          SamplerType.DISK, SamplerType.GPU_POWER,
                          SamplerType.INTERRUPTS, SamplerType.NETWORK,
                          SamplerType.TASKS, SamplerType.THERMAL)

  @classmethod
  def config_parser(cls) -> ProbeConfigParser:
    parser = super().config_parser()
    parser.add_argument(
        "sampling_interval",
        type=cli_helper.Duration.parse_non_zero,
        default=1000)
    parser.add_argument(
        "samplers", type=SamplerType, default=cls.SAMPLERS, is_list=True)
    return parser

  def __init__(self,
               sampling_interval: dt.timedelta = dt.timedelta(),
               samplers: Sequence[SamplerType] = SAMPLERS):
    super().__init__()
    self._sampling_interval = sampling_interval
    if sampling_interval.total_seconds() < 0:
      raise ValueError(f"Invalid sampling_interval={sampling_interval}")
    self._samplers = tuple(samplers)

  @property
  def key(self) -> Tuple[Tuple, ...]:
    return super().key + (
        ("sampling_interval", self.sampling_interval.total_seconds()),
        ("samplers", tuple(map(str, self.samplers))),
    )

  @property
  def sampling_interval(self) -> dt.timedelta:
    return self._sampling_interval

  @property
  def samplers(self) -> Tuple[SamplerType, ...]:
    return self._samplers

  def validate_browser(self, env: HostEnvironment, browser: Browser) -> None:
    super().validate_browser(env, browser)
    self.expect_macos(browser)

  def get_context(self, run: Run) -> PowerMetricsProbeContext:
    return PowerMetricsProbeContext(self, run)


class PowerMetricsProbeContext(ProbeContext[PowerMetricsProbe]):

  def __init__(self, probe: PowerMetricsProbe, run: Run) -> None:
    super().__init__(probe, run)
    self._power_metrics_process: Optional[subprocess.Popen] = None
    self._output_plist_file: pathlib.Path = self.result_path.with_suffix(
        ".plist")

  def start(self) -> None:
    self._power_metrics_process = self.browser_platform.popen(
        "sudo",
        "powermetrics",
        "-f",
        "plist",
        f"--samplers={','.join(map(str, self.probe.samplers))}",
        "-i",
        f"{int(self.probe.sampling_interval.total_seconds())}",
        "--output-file",
        self._output_plist_file,
        stdout=subprocess.DEVNULL)
    if self._power_metrics_process.poll():
      raise ValueError("Could not start powermetrics")
    atexit.register(self.stop_process)

  def stop(self) -> None:
    if self._power_metrics_process:
      self._power_metrics_process.terminate()

  def tear_down(self) -> ProbeResult:
    self.stop_process()
    return self.browser_result(file=(self._output_plist_file,))

  def stop_process(self) -> None:
    if self._power_metrics_process:
      helper.wait_and_kill(self._power_metrics_process)
      self._power_metrics_process = None
