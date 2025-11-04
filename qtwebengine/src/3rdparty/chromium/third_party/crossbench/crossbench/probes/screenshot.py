# Copyright 2024 The Chromium Authors
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

from __future__ import annotations

import datetime as dt
from typing import TYPE_CHECKING, List, Optional

from crossbench.probes.probe import (Probe, ProbeConfigParser, ProbeContext,
                                     ProbeMissingDataError)
from crossbench.probes.result_location import ResultLocation
from crossbench.probes.results import EmptyProbeResult, ProbeResult

if TYPE_CHECKING:
  from crossbench.browsers.browser import Viewport
  from crossbench.env import HostEnvironment
  from crossbench.path import RemotePath
  from crossbench.runner.groups import BrowsersRunGroup, RepetitionsRunGroup
  from crossbench.runner.run import Run


class ScreenshotProbe(Probe):
  """
  General-purpose Probe that collects screenshots.
  """
  NAME = "screenshot"
  RESULT_LOCATION = ResultLocation.BROWSER
  IMAGE_FORMAT = "png"

  @classmethod
  def config_parser(cls) -> ProbeConfigParser:
    parser = super().config_parser()
    # TODO: support interval-based screenshots
    return parser

  def _pre_check_viewport_size(self, env: HostEnvironment) -> None:
    for browser in env.runner.browsers:
      viewport: Viewport = browser.viewport
      if viewport.is_headless:
        env.handle_warning(
            f"Cannot take screenshots for headless browser: {browser}")
      if viewport.x < 10 or viewport.y < 50:
        env.handle_warning(
            f"Viewport for '{browser}' might include toolbar: {viewport}")

  def get_context(self, run: Run) -> ScreenshotProbeContext:
    return ScreenshotProbeContext(self, run)

  def merge_repetitions(self, group: RepetitionsRunGroup) -> ProbeResult:
    # TODO: implement
    return EmptyProbeResult()

  def merge_browsers(self, group: BrowsersRunGroup) -> ProbeResult:
    # TODO: implement
    return EmptyProbeResult()


class ScreenshotProbeContext(ProbeContext[ScreenshotProbe]):

  def __init__(self, probe: ScreenshotProbe, run: Run) -> None:
    super().__init__(probe, run)
    self._results: List[RemotePath] = []

  def get_default_result_path(self) -> RemotePath:
    screenshot_dir = super().get_default_result_path()
    self.browser_platform.mkdir(screenshot_dir)
    return screenshot_dir

  def start(self) -> None:
    self.screenshot("start")

  def start_story_run(self) -> None:
    self.screenshot("start_story")

  def stop_story_run(self) -> None:
    self.screenshot("stop_story")

  def stop(self) -> None:
    self.screenshot("stop")

  def screenshot(self, label: Optional[str] = None) -> None:
    # TODO: support screen coordinates
    if not label:
      label = str(dt.datetime.now().strftime("%Y-%m-%d_%H%M%S"))
    path = self.result_path / f"{label}.{ScreenshotProbe.IMAGE_FORMAT}"
    # TODO: use the browser's implementation first which might be more portable
    self.browser_platform.screenshot(path)
    self._results.append(path)

  def teardown(self) -> ProbeResult:
    if not self.browser_platform.is_dir(self.result_path):
      raise ProbeMissingDataError(
          f"No screen shot found at: {self.result_path}")
    return self.browser_result(file=tuple(self._results))
