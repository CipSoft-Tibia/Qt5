# Copyright 2022 The Chromium Authors
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

from __future__ import annotations

import atexit
import csv
import datetime as dt
import enum
import logging
import pathlib
import subprocess
from typing import TYPE_CHECKING, Optional, Sequence, Tuple

from crossbench import cli_helper, compat, helper
from crossbench.probes.probe import (Probe, ProbeConfigParser, ProbeContext,
                                     ResultLocation)
from crossbench.probes.results import ProbeResult

if TYPE_CHECKING:
  from crossbench.browsers.browser import Browser
  from crossbench.env import HostEnvironment
  from crossbench.runner.run import Run


@enum.unique
class SamplerType(compat.StrEnumWithHelp):
  MAIN_DISPLAY = ("main_display",
                  "Samples the backlight level of the main display.")
  BATTERY = ("battery", "Provides data retrieved from the IOPMPowerSource.")
  SMC = ("smc", ("Samples power usage from various hardware components "
                 "from the System Management Controller (SMC)"))
  M1 = ("m1", "Samples the temperature of M1 P-Cores and E-Cores.")
  USER_IDLE_LEVEL = (
      "user_idle_level",
      "Samples the machdep.user_idle_level sysctl value if it exists")
  RESOURCE_COALITION = ("resource_coalition", (
      "Provides resource usage data for a group of tasks that are part of a "
      "'resource coalition', including those that have died."))


class PowerSamplerProbe(Probe):
  """
  Probe for chrome's power_sampler helper binary to collect MacOS specific
  battery and system usage metrics.
  Note that the battery monitor only gets a value infrequently (> 30s), thus
  this probe mostly makes sense for long-running benchmarks.
  """

  NAME = "powersampler"
  RESULT_LOCATION = ResultLocation.BROWSER
  BATTERY_ONLY: bool = True
  SAMPLERS: Tuple[SamplerType,
                  ...] = (SamplerType.SMC, SamplerType.USER_IDLE_LEVEL,
                          SamplerType.MAIN_DISPLAY)

  @classmethod
  def config_parser(cls) -> ProbeConfigParser:
    parser = super().config_parser()
    parser.add_argument("bin_path", type=pathlib.Path)
    parser.add_argument(
        "sampling_interval",
        type=cli_helper.Duration.parse_non_zero,
        default=dt.timedelta(seconds=10))
    parser.add_argument(
        "samplers", type=SamplerType, default=cls.SAMPLERS, is_list=True)
    parser.add_argument(
        "wait_for_battery",
        type=bool,
        default=True,
        help="Wait for the first non-100% battery measurement before "
        "running the benchmark to ensure accurate readings.")
    return parser

  def __init__(self,
               bin_path: pathlib.Path,
               sampling_interval: dt.timedelta = dt.timedelta(),
               samplers: Sequence[SamplerType] = SAMPLERS,
               wait_for_battery: bool = True):
    super().__init__()
    self._bin_path = bin_path
    assert self._bin_path.exists(), ("Could not find power_sampler binary at "
                                     f"'{self._bin_path}'")
    self._sampling_interval = sampling_interval
    if sampling_interval.total_seconds() < 0:
      raise ValueError(f"Invalid sampling_interval={sampling_interval}")
    assert SamplerType.BATTERY not in samplers
    self._samplers = tuple(samplers)
    self._wait_for_battery = wait_for_battery

  @property
  def key(self) -> Tuple[Tuple, ...]:
    return super().key + (
        ("bin_path", str(self.bin_path)),
        ("sampling_interval", self.sampling_interval.total_seconds()),
        ("samplers", tuple(map(str, self.samplers))),
        ("wait_for_battery", self.wait_for_battery),
    )

  @property
  def bin_path(self) -> pathlib.Path:
    return self._bin_path

  @property
  def sampling_interval(self) -> dt.timedelta:
    return self._sampling_interval

  @property
  def samplers(self) -> Tuple[SamplerType, ...]:
    return self._samplers

  @property
  def wait_for_battery(self) -> bool:
    return self._wait_for_battery

  def validate_browser(self, env: HostEnvironment, browser: Browser) -> None:
    self.expect_macos(browser)
    if not browser.platform.is_battery_powered:
      env.handle_warning("Power Sampler only works on battery power, "
                         f"but Browser {browser} is connected to power.")
    # TODO() warn when external monitors are connected
    # TODO() warn about open terminals

  def get_context(self, run: Run) -> PowerSamplerProbeContext:
    return PowerSamplerProbeContext(self, run)


class PowerSamplerProbeContext(ProbeContext[PowerSamplerProbe]):

  def __init__(self, probe: PowerSamplerProbe, run: Run) -> None:
    super().__init__(probe, run)
    self._bin_path: pathlib.Path = probe.bin_path
    self._active_user_process: Optional[subprocess.Popen] = None
    self._power_process: Optional[subprocess.Popen] = None
    self._power_battery_process: Optional[subprocess.Popen] = None
    self._power_output: pathlib.Path = self.result_path.with_suffix(
        ".power.json")
    self._power_battery_output: pathlib.Path = self.result_path.with_suffix(
        ".power_battery.json")

  def setup(self) -> None:
    self._active_user_process = self.browser_platform.popen(
        self._bin_path,
        "--no-samplers",
        "--simulate-user-active",
        stdout=subprocess.DEVNULL)
    if self._active_user_process.poll():
      raise ValueError("Could not start active user background sampler")
    atexit.register(self.stop_processes)
    if self.probe.wait_for_battery:
      self._wait_for_battery_not_full(self.run)

  def start(self) -> None:
    assert self._active_user_process
    if sampling_interval := self.probe.sampling_interval.total_seconds():
      self._power_process = self.browser_platform.popen(
          self._bin_path,
          f"--sample-interval={int(sampling_interval)}",
          f"--samplers={','.join(map(str, self.probe.samplers))}",
          f"--json-output-file={self._power_output}",
          f"--resource-coalition-pid={self.browser_pid}",
          stdout=subprocess.DEVNULL)
      if self._power_process.poll():
        raise ValueError("Could not start power sampler")
    self._power_battery_process = self.browser_platform.popen(
        self._bin_path,
        "--sample-on-notification",
        f"--samplers={','.join(map(str, self.probe.samplers))+',battery'}",
        f"--json-output-file={self._power_battery_output}",
        f"--resource-coalition-pid={self.browser_pid}",
        stdout=subprocess.DEVNULL)
    if self._power_battery_process.poll():
      raise ValueError("Could not start power and battery sampler")

  def stop(self) -> None:
    if self._power_process:
      self._power_process.terminate()
    if self._power_battery_process:
      self._power_battery_process.terminate()

  def tear_down(self) -> ProbeResult:
    self.stop_processes()
    if self.probe.sampling_interval:
      return self.browser_result(
          json=[self._power_output, self._power_battery_output])
    return self.browser_result(json=[self._power_battery_output])

  def stop_processes(self) -> None:
    if self._power_process:
      helper.wait_and_kill(self._power_process)
      self._power_process = None
    if self._power_battery_process:
      helper.wait_and_kill(self._power_battery_process)
      self._power_battery_process = None
    if self._active_user_process:
      helper.wait_and_kill(self._active_user_process)
      self._active_user_process = None

  def _wait_for_battery_not_full(self, run: Run) -> None:
    """
    Empirical evidence has shown that right after a full battery charge, the
    current capacity stays equal to the maximum capacity for several minutes,
    despite the fact that power is definitely consumed. To ensure that power
    consumption estimates from battery level are meaningful, wait until the
    battery is no longer reporting being fully charged before crossbench.
    """
    del run
    logging.info("POWER SAMPLER: Waiting for non-100% battery or "
                 "initial sample to synchronize")
    while True:
      assert self.browser_platform.is_battery_powered, (
          "Cannot wait for draining if power is connected.")

      power_sampler_output = self.browser_platform.sh_stdout(
          self._bin_path, "--sample-on-notification", "--samplers=battery",
          "--sample-count=1")

      for row in csv.DictReader(power_sampler_output.splitlines()):
        max_capacity = float(row["battery_max_capacity(Ah)"])
        current_capacity = float(row["battery_current_capacity(Ah)"])
        percent = 100 * current_capacity / max_capacity
        logging.debug("POWER SAMPLER: Battery level is %.2f%%", percent)
        if max_capacity != current_capacity:
          return
