# Copyright 2024 The Chromium Authors
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

from __future__ import annotations

import abc
import datetime as dt
from typing import TYPE_CHECKING, List, Optional, Tuple, cast

from crossbench.action_runner.android_input_action_runner import \
    AndroidInputActionRunner
from crossbench.action_runner.base import ActionRunner
from crossbench.action_runner.chromeos_input_action_runner import \
    ChromeOSInputActionRunner
from crossbench.action_runner.default_action_runner import DefaultActionRunner
from crossbench.benchmarks.loading.playback_controller import \
    PlaybackController
from crossbench.benchmarks.loading.tab_controller import TabController
from crossbench.stories.story import Story

if TYPE_CHECKING:
  from crossbench.benchmarks.loading.loading_benchmark import LoadingBenchmark
  from crossbench.cli.config.secrets import Secrets
  from crossbench.runner.run import Run

DEFAULT_DURATION_SECONDS = 15
DEFAULT_DURATION = dt.timedelta(seconds=DEFAULT_DURATION_SECONDS)

# This is initialized in interactive.py to avoid circular dependencies
PAGE_LIST: List[Page] = []

class Page(Story, metaclass=abc.ABCMeta):

  @classmethod
  def all_story_names(cls) -> Tuple[str, ...]:
    assert PAGE_LIST, "Missing predefined page list"
    # TODO: move all story names magic to the dedicated StoryFilter.
    # Use module instead of direct import to avoid import cycle
    return tuple(page.name for page in PAGE_LIST)

  def __init__(self,
               name: str,
               duration: dt.timedelta = DEFAULT_DURATION,
               playback: PlaybackController = PlaybackController.default(),
               tabs: TabController = TabController.default(),
               about_blank_duration: dt.timedelta = dt.timedelta(),
               secrets: Optional[Secrets] = None):
    self._playback: PlaybackController = playback
    self._tabs: TabController = tabs
    self._about_blank_duration = about_blank_duration
    super().__init__(name, duration, secrets)

  @property
  def about_blank_duration(self) -> dt.timedelta:
    return self._about_blank_duration

  def set_parent(self, parent: Page) -> None:
    # TODO: support nested playback controllers.
    self._playback = PlaybackController.default()
    self._tabs = TabController.default()
    del parent

  @abc.abstractmethod
  def run_with(self, run: Run, action_runner: ActionRunner,
               multiple_tabs: bool) -> None:
    pass

  @property
  @abc.abstractmethod
  def first_url(self) -> str:
    pass

  @property
  def tabs(self) -> TabController:
    return self._tabs


def get_action_runner(run: Run) -> ActionRunner:
  # TODO: make sure we have a single instance per Run
  benchmark = cast("LoadingBenchmark", run.benchmark)

  if not benchmark.action_runner:
    platform = run.browser.platform

    if platform.is_android:
      benchmark.action_runner = AndroidInputActionRunner()
    elif platform.is_chromeos:
      benchmark.action_runner = ChromeOSInputActionRunner()
    else:
      benchmark.action_runner = DefaultActionRunner()

  return benchmark.action_runner
