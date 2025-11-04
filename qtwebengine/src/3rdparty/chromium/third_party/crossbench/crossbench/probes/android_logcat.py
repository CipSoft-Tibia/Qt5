# Copyright 2024 The Chromium Authors
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

from __future__ import annotations

from typing import TYPE_CHECKING, Iterable, Optional, Tuple, cast

from crossbench.plt.android_adb import AndroidAdbPlatform
from crossbench.probes.probe import (Probe, ProbeConfigParser, ProbeContext,
                                     ProbeIncompatibleBrowser, ResultLocation)
from crossbench.probes.results import LocalProbeResult, ProbeResult

if TYPE_CHECKING:
  from crossbench.browsers.browser import Browser
  from crossbench.env import HostEnvironment
  from crossbench.runner.run import Run


class AndroidLogcatProbe(Probe):
  """
  Android-only probe to collect logcat traces.
  """
  NAME = "logcat"
  RESULT_LOCATION = ResultLocation.LOCAL

  IS_GENERAL_PURPOSE = True

  @classmethod
  def config_parser(cls) -> ProbeConfigParser:
    parser = super().config_parser()
    parser.add_argument(
        "filterspec",
        type=str,
        is_list=True,
        default=tuple(),
        help="Filter specifications are a series of <tag>[:priority]")
    return parser

  def __init__(self, filterspec: Iterable[str]):
    super().__init__()
    self._filterspec = tuple(filterspec)

  @property
  def filterspec(self) -> Tuple[str, ...]:
    return self._filterspec

  def validate_browser(self, env: HostEnvironment, browser: Browser) -> None:
    super().validate_browser(env, browser)
    if not browser.platform.is_android:
      raise ProbeIncompatibleBrowser(self, browser, "Only supported on android")

  def get_context(self, run: Run) -> AndroidLogcatProbeContext:
    return AndroidLogcatProbeContext(self, run)


class AndroidLogcatProbeContext(ProbeContext[AndroidLogcatProbe]):

  def __init__(self, probe: AndroidLogcatProbe, run: Run) -> None:
    super().__init__(probe, run)
    self._start_time: Optional[str] = None

  def _get_browser_platform_time(self) -> str:
    return self.browser_platform.sh_stdout("date",
                                           "+%Y-%m-%d %H:%M:%S").rstrip()

  def _log_to_logcat(self, msg: str):
    self.browser_platform.sh("log", "-t", "crossbench", msg)

  @property
  def browser_platform(self) -> AndroidAdbPlatform:
    browser_platform = super().browser_platform
    assert isinstance(browser_platform, AndroidAdbPlatform)
    return cast(AndroidAdbPlatform, browser_platform)

  def start(self) -> None:
    self._start_time = self._get_browser_platform_time()
    self._log_to_logcat("logcat probe start")

  def stop(self) -> None:
    self._log_to_logcat("logcat probe end")

  def teardown(self) -> ProbeResult:
    assert self._start_time
    file = self.local_result_path.with_suffix(".txt")
    with file.open("w", encoding="utf-8") as f:
      self.runner_platform.sh(
          "adb",
          "logcat",
          "-t",
          self._start_time + ".000",
          *self.probe.filterspec,
          stdout=f)

    return LocalProbeResult(trace=(file,))
