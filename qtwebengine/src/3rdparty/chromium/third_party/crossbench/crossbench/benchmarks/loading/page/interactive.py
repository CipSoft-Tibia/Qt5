# Copyright 2024 The Chromium Authors
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

from __future__ import annotations

import datetime as dt
import logging
from typing import TYPE_CHECKING, Optional, Tuple, cast

from crossbench.action_runner.action.action_type import ActionType
from crossbench.action_runner.action.get import GetAction
from crossbench.benchmarks.loading.page.base import Page, get_action_runner
from crossbench.benchmarks.loading.playback_controller import \
    PlaybackController
from crossbench.benchmarks.loading.tab_controller import TabController

if TYPE_CHECKING:
  from crossbench.action_runner.base import ActionRunner
  from crossbench.benchmarks.loading.config.blocks import ActionBlock
  from crossbench.benchmarks.loading.config.login.custom import LoginBlock
  from crossbench.cli.config.secrets import Secrets
  from crossbench.runner.run import Run
  from crossbench.types import JsonDict


class InteractivePage(Page):

  def __init__(self,
               name: str,
               blocks: Tuple[ActionBlock, ...],
               setup: Optional[ActionBlock] = None,
               login: Optional[LoginBlock] = None,
               secrets: Optional[Secrets] = None,
               playback: PlaybackController = PlaybackController.default(),
               tabs: TabController = TabController.default(),
               about_blank_duration: dt.timedelta = dt.timedelta(),
               run_login: bool = True,
               run_setup: bool = True):
    assert name, "missing name"
    self._name: str = name
    assert isinstance(blocks, tuple)
    self._blocks: Tuple[ActionBlock, ...] = blocks
    assert self._blocks, "Must have at least 1 valid action"
    assert not any(block.is_login for block in blocks), (
        "No login blocks allowed as normal action block")
    self._setup_block = setup
    self._login_block = login
    self._run_login = run_login
    self._run_setup = run_setup
    duration = self._get_duration()
    super().__init__(self._name, duration, playback, tabs, about_blank_duration,
                     secrets)

  @property
  def login_block(self) -> Optional[ActionBlock]:
    return self._login_block

  @property
  def setup_block(self) -> Optional[ActionBlock]:
    return self._setup_block

  @property
  def blocks(self) -> Tuple[ActionBlock, ...]:
    return self._blocks

  @property
  def first_url(self) -> str:
    for block in self.blocks:
      for action in block:
        if action.TYPE == ActionType.GET:
          return cast(GetAction, action).url
    raise RuntimeError("No GET action with an URL found.")

  def create_failure_artifacts(self,
                               run: Run,
                               message: str = "failure") -> None:
    action_runner = get_action_runner(run)
    try:
      action_runner.failure_screenshot(run, message)
    except Exception as e:  # pylint: disable=broad-except
      logging.error("Failed to take a failure screenshot: %s", str(e))

    try:
      action_runner.dump_html_impl(run, message)
    except Exception as e:  # pylint: disable=broad-except
      logging.error("Failed to dump HTML on failure: %s", str(e))

  def setup(self, run: Run) -> None:
    action_runner = get_action_runner(run)
    if self._run_login and (login_block := self.login_block):
      action_runner.run_login(run, self, login_block)
    if self._run_setup and (setup_block := self.setup_block):
      action_runner.run_setup(run, self, setup_block)

  def teardown(self, run: Run) -> None:
    action_runner = get_action_runner(run)
    action_runner.teardown(run)

  def run(self, run: Run) -> None:
    action_runner = get_action_runner(run)
    multiple_tabs = self.tabs.multiple_tabs
    for _ in self._playback:
      action_runner.run_interactive_page(run, self, multiple_tabs)

  def run_with(self, run: Run, action_runner: ActionRunner,
               multiple_tabs: bool) -> None:
    action_runner.run_interactive_page(run, self, multiple_tabs)

  def details_json(self) -> JsonDict:
    result = super().details_json()
    result["actions"] = list(block.to_json() for block in self._blocks)
    return result

  def _get_duration(self) -> dt.timedelta:
    duration = dt.timedelta()
    for block in self._blocks:
      duration += block.duration
    return duration
