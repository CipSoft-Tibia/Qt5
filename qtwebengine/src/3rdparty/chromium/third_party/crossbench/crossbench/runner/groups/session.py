# Copyright 2023 The Chromium Authors
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

from __future__ import annotations

import contextlib
import enum
import logging
from typing import TYPE_CHECKING, Iterable, Iterator, List, Optional, Tuple

from crossbench import helper
from crossbench.flags import Flags, JSFlags
from crossbench.probes.results import EmptyProbeResult

from .base import RunGroup

if TYPE_CHECKING:
  import pathlib

  from selenium.webdriver.common.options import ArgOptions

  from crossbench import exception, plt
  from crossbench.browsers.browser import Browser
  from crossbench.probes.probe import Probe
  from crossbench.probes.results import ProbeResult
  from crossbench.runner.run import Run
  from crossbench.network.base import Network
  from crossbench.runner.runner import Runner
  from crossbench.runner.timing import Timing
  from crossbench.types import JsonDict


@enum.unique
class _State(enum.IntEnum):
  BUILDING = enum.auto()
  READY = enum.auto()
  STARTING = enum.auto()
  RUNNING = enum.auto()
  STOPPING = enum.auto()
  DONE = enum.auto()


class BrowserSessionRunGroup(RunGroup):
  """
  Groups Run objects together that are run within the same browser session.
  At the beginning of a new session the caches are cleared and the
  browser is (re-)started.
  """

  def __init__(self, runner: Runner, browser: Browser, index: int,
               root_dir: pathlib.Path, throw: bool) -> None:
    super().__init__(throw)
    self._state: _State = _State.BUILDING
    self._runner = runner
    self._durations = helper.Durations()
    self._browser = browser
    self._network: Network = browser.network
    self._index = index
    self._runs: List[Run] = []
    self._root_dir: pathlib.Path = root_dir
    self._browser_tmp_dir: Optional[pathlib.Path] = None
    self._extra_js_flags = JSFlags()
    self._extra_flags = Flags()

  def append(self, run: Run) -> None:
    assert self._state == _State.BUILDING
    assert run.browser_session == self
    assert run.browser is self._browser
    # TODO: assert that the runs have compatible flags (likely we're only
    # allowing changes in the cache temperature)
    # TODO: Add session/run switch for probe results
    self._runs.append(run)

  def set_ready(self) -> None:
    assert self._state == _State.BUILDING
    self._state = _State.READY
    self._validate()
    self._set_path(self._get_session_dir())

  def _validate(self) -> None:
    if not self._runs:
      raise ValueError("BrowserSessionRunGroup must be non-empty.")
    self._validate_same_browser_probes()

  def _validate_same_browser_probes(self) -> None:
    first_run = self._runs[0]
    first_probes = tuple(first_run.probes)
    for index, run in enumerate(self.runs):
      if first_run.browser is not run.browser:
        raise ValueError("A browser session can only contain "
                         "Runs with the same Browser.\n"
                         f"runs[0].browser == {first_run.browser} vs. "
                         f"runs[{index}].browser == {run.browser}")
      if first_probes != tuple(run.probes):
        raise ValueError("Got conflicting Probes within a browser session.")

  @property
  def raw_sessions_dir(self) -> pathlib.Path:
    return (self.root_dir / self.browser.unique_name / "sessions" /
            str(self.index))

  @property
  def is_single_run(self) -> bool:
    return len(self._runs) == 1

  @property
  def first_run(self) -> Run:
    return self._runs[0]

  def _get_session_dir(self) -> pathlib.Path:
    if self.is_single_run:
      return self.first_run.out_dir
    if not self._runs:
      raise ValueError("Cannot have empty browser session")
    return self.raw_sessions_dir

  @property
  def runner(self) -> Runner:
    return self._runner

  @property
  def runner_platform(self) -> plt.Platform:
    return self.runner.platform

  def network(self) -> Network:
    return self._network

  @property
  def browser(self) -> Browser:
    return self._browser

  @property
  def index(self) -> int:
    return self._index

  @property
  def browser_platform(self) -> plt.Platform:
    return self._browser.platform

  @property
  def is_running(self) -> bool:
    return self._state == _State.RUNNING

  @property
  def is_remote(self) -> bool:
    return self.browser_platform.is_remote

  @property
  def root_dir(self) -> pathlib.Path:
    return self._root_dir

  @property
  def runs(self) -> Iterable[Run]:
    return iter(self._runs)

  @property
  def timing(self) -> Timing:
    return self._runs[0].timing

  @property
  def extra_js_flags(self) -> JSFlags:
    assert self._state < _State.RUNNING
    return self._extra_js_flags

  @property
  def extra_flags(self) -> Flags:
    assert self._state < _State.RUNNING
    return self._extra_flags

  def add_flag_details(self, details_json: JsonDict) -> None:
    assert isinstance(details_json["js_flags"], (list, tuple))
    details_json["js_flags"] += tuple(self._extra_js_flags.get_list())
    assert isinstance(details_json["flags"], (list, tuple))
    details_json["flags"] += tuple(self._extra_flags.get_list())

  def setup_selenium_options(self, options: ArgOptions):
    # Using only the first run, since all runs need to have the same probes.
    for probe_context in self.first_run.probe_contexts:
      probe_context.setup_selenium_options(options)

  @property
  def info_stack(self) -> exception.TInfoStack:
    return ("Merging results from multiple browser sessions",
            f"browser={self.browser.unique_name}", f"session={self.index}")

  @property
  def info(self) -> JsonDict:
    info_dict = super().info
    info_dict.update({"index": self.index})
    return info_dict

  @property
  def browser_tmp_dir(self) -> pathlib.Path:
    if not self._browser_tmp_dir:
      prefix = f"cb_browser_session_{self.index}"
      self._browser_tmp_dir = self.browser_platform.mkdtemp(prefix)
    return self._browser_tmp_dir

  @contextlib.contextmanager
  def measure(
      self, label: str
  ) -> Iterator[Tuple[exception.ExceptionAnnotationScope,
                      helper.DurationMeasureContext]]:
    # Return a combined context manager that adds a named exception info
    # and measures the time during the with-scope.
    with self._exceptions.info(label) as stack, self._durations.measure(
        label) as timer:
      yield (stack, timer)

  def merge(self, runner: Runner) -> None:
    # TODO: implement merging of session probes
    pass

  def _merge_probe_results(self, probe: Probe) -> ProbeResult:
    return EmptyProbeResult()

  @contextlib.contextmanager
  def open(self, is_dry_run: bool = False) -> Iterator[BrowserSessionRunGroup]:
    self._setup_session_dir()
    with helper.ChangeCWD(self.path):
      try:
        self._setup(is_dry_run)
        yield self
      finally:
        self._teardown(is_dry_run)

  def _setup(self, is_dry_run: bool) -> None:
    assert self._state == _State.READY
    self._state = _State.STARTING
    with self.measure("browser-session-setup"):
      self._setup_runs(is_dry_run)
      self._start_browser(is_dry_run)
      self._state = _State.RUNNING

  def _setup_session_dir(self):
    self.path.mkdir(parents=True, exist_ok=True)
    if self.runner_platform.is_win:
      logging.debug("Skipping session_dir symlink on windows.")
      return
    if self.is_single_run:
      # If there is a single run per session we reuse the run-dir.
      self.raw_sessions_dir.parent.mkdir(parents=True, exist_ok=True)
      self.raw_sessions_dir.symlink_to(self.path)

  def _setup_runs(self, is_dry_run: bool) -> None:
    # TODO: handle session vs run probe.
    for run in self.runs:
      logging.info("Preparing SESSION %s RUN %s", self.index, run.index)
      run.setup(is_dry_run)

  def _start_browser(self, is_dry_run: bool) -> None:
    assert self._state == _State.STARTING
    if is_dry_run:
      logging.info("BROWSER: %s", self.browser.path)
      return

    browser_log_file = self.path / "browser.log"
    assert not browser_log_file.exists(), (
        f"Default browser log file {browser_log_file} already exists.")
    self._browser.set_log_file(browser_log_file)

    with self.measure("browser-setup"):
      try:
        # pytype somehow gets the package path wrong here, disabling for now.
        self._browser.setup(self)  # pytype: disable=wrong-arg-types
      except Exception as e:
        logging.debug("Browser setup failed: %s", e)
        # Clean up half-setup browser instances
        self._browser.force_quit()
        raise

  def _teardown(self, is_dry_run: bool) -> None:
    assert self._state == _State.RUNNING or self._state == _State.STARTING
    self._state = _State.STOPPING
    if is_dry_run:
      return
    with self.measure("browser-session-teardown"):
      try:
        self._stop_browser()
      finally:
        assert self._state == _State.STOPPING
        self._state = _State.DONE

  def _stop_browser(self) -> None:
    assert self._state == _State.STOPPING
    # TODO: implement

  # TODO: remove once cleanly implemented
  def is_first_run(self, run: Run) -> bool:
    return self.first_run is run

  # TODO: remove once cleanly implemented
  def is_last_run(self, run: Run) -> bool:
    return self._runs[-1] is run
