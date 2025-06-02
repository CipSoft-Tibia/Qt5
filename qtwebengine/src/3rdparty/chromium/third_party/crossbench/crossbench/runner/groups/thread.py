# Copyright 2023 The Chromium Authors
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

from __future__ import annotations

import logging
import threading
from typing import Iterable, Tuple, TYPE_CHECKING

from ordered_set import OrderedSet

if TYPE_CHECKING:
  from crossbench.runner.run import Run
  from crossbench.runner.runner import Runner
  from .session import BrowserSessionRunGroup


class RunThreadGroup(threading.Thread):
  """The main interface to start Runs.
  - Typically only a single RunThreadGroup is used.
  - If runs are executed in parallel, multiple RunThreadGroup are used
  """

  def __init__(self, runs: Iterable[Run]) -> None:
    super().__init__()
    self._runs = tuple(runs)
    assert self._runs, "Got unexpected empty runs list"
    self._runner: Runner = self._runs[0].runner
    self._total_run_count = len(self._runner.runs)
    self._browser_sessions: OrderedSet[BrowserSessionRunGroup] = OrderedSet(
        run.browser_session for run in runs)
    self.is_dry_run: bool = False
    self._verify_contains_all_browser_session_runs()
    self._verify_same_runner()

  def _verify_contains_all_browser_session_runs(self) -> None:
    runs_set = set(self._runs)
    for browser_session in self._browser_sessions:
      for session_run in browser_session.runs:
        assert session_run in runs_set, (
            f"BrowserSession {browser_session} is not allowed to have "
            f"{session_run} in another RunThreadGroup.")

  def _verify_same_runner(self) -> None:
    for run in self._runs:
      assert run.runner is self._runner, "All Runs must have the same Runner."

  @property
  def runs(self) -> Tuple[Run, ...]:
    return tuple(self._runs)

  def _log_run(self, run: Run):
    logging.info("=" * 80)
    logging.info("RUN %s/%s", run.index + 1, self._total_run_count)
    logging.info("=" * 80)

  def run(self) -> None:
    for browser_session in self._browser_sessions:
      if browser_session.is_single_run:
        self._log_run(browser_session.first_run)
      else:
        logging.info("=" * 80)
      with browser_session.open():
        for run in browser_session.runs:
          if not browser_session.is_single_run:
            self._log_run(run)
          run.run(self.is_dry_run)
          if run.is_success:
            run.log_results()
          else:
            self._runner.exceptions.extend(run.exceptions)
