# Copyright 2023 The Chromium Authors
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

from __future__ import annotations

import contextlib
import datetime as dt
import enum
import logging
import pathlib
from typing import (TYPE_CHECKING, Any, Iterable, Iterator, List, Optional,
                    Tuple)

from crossbench import compat, exception, helper, plt
from crossbench.flags import Flags, JSFlags
from crossbench.probes import internal as internal_probe
from crossbench.probes.probe import ResultLocation
from crossbench.probes.results import (EmptyProbeResult, ProbeResult,
                                       ProbeResultDict)

from .actions import Actions
from .timing import Timing

if TYPE_CHECKING:
  from crossbench.browsers.browser import Browser
  from crossbench.env import HostEnvironment
  from crossbench.probes.probe import Probe, ProbeContext
  from crossbench.stories.story import Story
  from crossbench.types import JsonDict

  from .groups import BrowserSessionRunGroup
  from .runner import Runner


@enum.unique
class RunState(enum.IntEnum):
  INITIAL = enum.auto()
  SETUP = enum.auto()
  READY = enum.auto()
  RUN = enum.auto()
  DONE = enum.auto()


@enum.unique
class Temperature(compat.StrEnumWithHelp):
  COLD = ("cold", "first run")
  WARM = ("warm", "second run")
  HOT = ("hot", "third run")


class Run:

  def __init__(self,
               runner: Runner,
               browser_session: BrowserSessionRunGroup,
               story: Story,
               repetition: int,
               temperature: str,
               index: int,
               name: Optional[str] = None,
               timeout: dt.timedelta = dt.timedelta(),
               throw: bool = False):
    self._state: RunState = RunState.INITIAL
    self._runner = runner
    self._browser_session = browser_session
    self._browser: Browser = browser_session.browser
    browser_session.append(self)
    self._story = story
    assert repetition >= 0
    self._repetition = repetition
    assert temperature, "Missing cache-temperature value."
    self._temperature = temperature
    assert index >= 0
    self._index = index
    self._name = name
    self._out_dir = self._get_out_dir(browser_session.root_dir).absolute()
    self._probe_results = ProbeResultDict(self._out_dir)
    self._probe_contexts: List[ProbeContext] = []
    self._durations = helper.Durations()
    self._start_datetime = dt.datetime.utcfromtimestamp(0)
    self._timeout = timeout
    self._exceptions = exception.Annotator(throw)
    self._browser_tmp_dir: Optional[pathlib.Path] = None

  def __str__(self) -> str:
    return f"Run({self.name}, {self._state}, {self.browser})"

  def _get_out_dir(self, root_dir: pathlib.Path) -> pathlib.Path:
    return (root_dir / plt.safe_filename(self.browser.unique_name) / "stories" /
            plt.safe_filename(self.story.name) / str(self._repetition) /
            str(self._temperature))

  @property
  def group_dir(self) -> pathlib.Path:
    return self.out_dir.parent

  def actions(self,
              name: str,
              verbose: bool = False,
              measure: bool = True) -> Actions:
    return Actions(name, self, verbose=verbose, measure=measure)

  @property
  def info_stack(self) -> exception.TInfoStack:
    return (
        f"Run({self.name})",
        (f"browser={self.browser.type} label={self.browser.label} "
         "binary={self.browser.path}"),
        f"story={self.story}",
        f"repetition={self.repetition}",
    )

  def details_json(self) -> JsonDict:
    return {
        "name": self.name,
        "index": self.index,
        "repetition": self.repetition,
        "browser_session": self.browser_session.index,
        "temperature": self.temperature,
        "story": str(self.story),
        "probes": [probe.name for probe in self.probes],
        "duration": self.story.duration.total_seconds(),
        "startDateTime": str(self.start_datetime),
        "timeout": self.timeout.total_seconds(),
    }

  @property
  def temperature(self) -> str:
    return self._temperature

  @property
  def timing(self) -> Timing:
    return self.runner.timing

  @property
  def durations(self) -> helper.Durations:
    return self._durations

  @property
  def start_datetime(self) -> dt.datetime:
    return self._start_datetime

  def max_end_datetime(self) -> dt.datetime:
    if not self._timeout:
      return dt.datetime.max
    return self._start_datetime + self._timeout

  @property
  def timeout(self) -> dt.timedelta:
    return self._timeout

  @property
  def repetition(self) -> int:
    return self._repetition

  @property
  def index(self) -> int:
    return self._index

  @property
  def runner(self) -> Runner:
    return self._runner

  @property
  def browser_session(self) -> BrowserSessionRunGroup:
    return self._browser_session

  @property
  def browser(self) -> Browser:
    return self._browser

  @property
  def platform(self) -> plt.Platform:
    return self.browser_platform

  @property
  def environment(self) -> HostEnvironment:
    # TODO: replace with custom BrowserEnvironment
    return self.runner.env

  @property
  def browser_platform(self) -> plt.Platform:
    return self._browser.platform

  @property
  def runner_platform(self) -> plt.Platform:
    return self.runner.platform

  @property
  def is_remote(self) -> bool:
    return self.browser_platform.is_remote

  @property
  def out_dir(self) -> pathlib.Path:
    """A local directory where all result files are gathered.
    Results from browsers on remote platforms are transferred to this dir
    as well."""
    return self._out_dir

  @property
  def browser_tmp_dir(self) -> pathlib.Path:
    """Returns a path to a tmp dir on the browser platform."""
    if not self._browser_tmp_dir:
      prefix = "cb_run_results"
      self._browser_tmp_dir = self.browser_platform.mkdtemp(prefix)
    return self._browser_tmp_dir

  @property
  def results(self) -> ProbeResultDict:
    return self._probe_results

  @property
  def story(self) -> Story:
    return self._story

  @property
  def name(self) -> Optional[str]:
    return self._name

  @property
  def probes(self) -> Iterable[Probe]:
    return self._runner.probes

  @property
  def exceptions(self) -> exception.Annotator:
    return self._exceptions

  @property
  def is_success(self) -> bool:
    return self._exceptions.is_success

  @property
  def probe_contexts(self) -> Iterator[ProbeContext]:
    return iter(self._probe_contexts)

  @property
  def session(self) -> BrowserSessionRunGroup:
    return self._browser_session

  @contextlib.contextmanager
  def measure(
      self, label: str
  ) -> Iterator[Tuple[exception.ExceptionAnnotationScope,
                      helper.DurationMeasureContext]]:
    # Return a combined context manager that adds an named exception info
    # and measures the time during the with-scope.
    with self._exceptions.info(label) as stack, self._durations.measure(
        label) as timer:
      yield (stack, timer)

  def exception_info(self,
                     *stack_entries: str) -> exception.ExceptionAnnotationScope:
    return self._exceptions.info(*stack_entries)

  def exception_handler(
      self,
      *stack_entries: str,
      exceptions: exception.TExceptionTypes = (Exception,)
  ) -> exception.ExceptionAnnotationScope:
    return self._exceptions.capture(*stack_entries, exceptions=exceptions)

  def get_browser_details_json(self) -> JsonDict:
    details_json = self.browser.details_json()
    self.session.add_flag_details(details_json)
    return details_json

  def get_default_probe_result_path(self, probe: Probe) -> pathlib.Path:
    """Return a local or remote/browser-based result path depending on the
    Probe default RESULT_LOCATION."""
    if probe.RESULT_LOCATION == ResultLocation.BROWSER:
      return self.get_browser_probe_result_path(probe)
    if probe.RESULT_LOCATION == ResultLocation.LOCAL:
      return self.get_local_probe_result_path(probe)
    raise ValueError(f"Invalid probe.RESULT_LOCATION {probe.RESULT_LOCATION} "
                     f"for probe {probe}")

  def get_local_probe_result_path(self, probe: Probe) -> pathlib.Path:
    file = self._out_dir / probe.result_path_name
    assert not file.exists(), f"Probe results file exists already. file={file}"
    return file

  def get_browser_probe_result_path(self, probe: Probe) -> pathlib.Path:
    """Returns a temporary path on the remote browser or the same as
    get_local_probe_result_path() on a local browser."""
    if not self.is_remote:
      return self.get_local_probe_result_path(probe)
    path = self.browser_tmp_dir / probe.result_path_name
    self.browser_platform.mkdir(path.parent)
    logging.debug("Creating remote result dir=%s on platform=%s", path.parent,
                  self.browser_platform)
    return path

  def setup(self, is_dry_run: bool = False) -> None:
    self._advance_state(RunState.INITIAL, RunState.SETUP)
    self._setup_dirs()
    with helper.ChangeCWD(self._out_dir), self.exception_info(*self.info_stack):
      assert not self._probe_contexts
      try:
        self._probe_contexts = self._setup_probes(is_dry_run)
      except Exception as e:  # pylint: disable=broad-except
        self._handle_setup_error(e)
        return

  def _setup_dirs(self) -> None:
    self._start_datetime = dt.datetime.now()
    logging.debug("Creating Run(%s) out dir: %s", self, self._out_dir)
    self._out_dir.mkdir(parents=True, exist_ok=True)
    self._create_session_dir()

  def _create_session_dir(self) -> None:
    session_run_dir = self._out_dir / "session"
    assert not session_run_dir.exists(), (
        f"Cannot setup session dir twice: {session_run_dir}")
    if self.runner_platform.is_win:
      logging.debug("Skipping session_dir symlink on windows.")
      return
    # Source: BROWSER / "stories" / STORY / REPETITION / CACHE_TEMP / "session"
    # Target: BROWSER / "sessions" / SESSION
    relative_session_dir = (
        pathlib.Path("../../../..") /
        self.browser_session.path.relative_to(self.out_dir.parents[3]))
    session_run_dir.symlink_to(relative_session_dir)

  def _log_setup(self) -> None:
    logging.debug("SETUP")
    logging.info("PROBES: %s", ", ".join(probe.NAME for probe in self.probes))
    self.story.log_run_details(self)
    logging.info("RUN DIR: %s", self._out_dir)
    logging.debug("CWD %s", self._out_dir)

  def _setup_probes(self, is_dry_run: bool) -> List[ProbeContext[Any]]:
    assert self._state == RunState.SETUP
    self._log_setup()
    if is_dry_run:
      return []

    with self.measure("runner-cooldown"):
      self._runner.wait(self._runner.timing.cool_down_time, absolute_time=True)
      self._runner.cool_down()

    probe_run_contexts: List[ProbeContext] = []
    with self.measure("probes-creation"):
      probe_set = set()
      for probe in self.probes:
        assert probe not in probe_set, (
            f"Got duplicate probe name={probe.name}")
        probe_set.add(probe)
        if probe.PRODUCES_DATA:
          self._probe_results[probe] = EmptyProbeResult()
        assert probe.is_attached, (
            f"Probe {probe.name} is not properly attached to a browser")
        probe_run_contexts.append(probe.get_context(self))

    with self.measure("probes-setup"):
      for probe_context in probe_run_contexts:
        with self.measure(f"probes-setup {probe_context.name}"):
          probe_context.setup()  # pytype: disable=wrong-arg-types
    return probe_run_contexts

  def _handle_setup_error(self, setup_exception: BaseException) -> None:
    self._advance_state(RunState.SETUP, RunState.DONE)
    self._exceptions.append(setup_exception)
    assert self._state == RunState.DONE
    assert not self._exceptions.is_success
    # Special handling for crucial runner probes
    internal_probe_contexts = [
        context for context in self._probe_contexts
        if isinstance(context.probe, internal_probe.InternalProbe)
    ]
    self._tear_down_probe_contexts(internal_probe_contexts)

  def run(self, is_dry_run: bool = False) -> None:
    self._advance_state(RunState.SETUP, RunState.READY)
    self._start_datetime = dt.datetime.now()
    with helper.ChangeCWD(self._out_dir), self.exception_info(*self.info_stack):
      assert self._probe_contexts
      try:
        self._run(is_dry_run)
      except Exception as e:  # pylint: disable=broad-except
        self._exceptions.append(e)
      finally:
        if not is_dry_run:
          self.tear_down()

  def _run(self, is_dry_run: bool) -> None:
    self._advance_state(RunState.READY, RunState.RUN)

    self.browser.splash_screen.run(self)
    assert self._probe_contexts
    probe_start_time = dt.datetime.now()
    probe_context_manager = contextlib.ExitStack()

    for probe_context in self._probe_contexts:
      probe_context.set_start_time(probe_start_time)
      probe_context_manager.enter_context(probe_context)

    with probe_context_manager:
      self._durations["probes-start"] = dt.datetime.now() - probe_start_time
      logging.info("RUNNING STORY")
      assert self._state == RunState.RUN, "Invalid state"
      try:
        with self.measure("run"), helper.Spinner():
          if not is_dry_run:
            self._run_story_setup()
            self._story.run(self)
            self._run_story_tear_down()
      except TimeoutError as e:
        # Handle TimeoutError earlier since they might be caused by
        # throttled down non-foreground browser.
        self._exceptions.append(e)
      self.environment.check_browser_focused(self.browser)

  def _run_story_setup(self) -> None:
    with self.measure("story-setup"):
      self._story.setup(self)
    with self.measure("probes-start_story_run"):
      for probe_context in self._probe_contexts:
        with self.exception_handler(
            f"Probe {probe_context.name} start_story_run"):
          probe_context.start_story_run()

  def _run_story_tear_down(self) -> None:
    with self.measure("probes-stop_story_run"):
      for probe_context in self._probe_contexts:
        with self.exception_handler(
            f"Probe {probe_context.name} stop_story_run"):
          probe_context.stop_story_run()
    with self.measure("story-tear-down"):
      self._story.tear_down(self)

  def _advance_state(self, expected: RunState, next_state: RunState) -> None:
    assert self._state == expected, (
        f"Invalid state got={self._state} expected={expected}")
    self._state = next_state

  def tear_down(self, is_shutdown: bool = False) -> None:
    self._advance_state(RunState.RUN, RunState.DONE)
    self._tear_down_browser(is_shutdown)

    with self.measure("probes-tear_down"):
      self._tear_down_probe_contexts(self._probe_contexts)
      self._probe_contexts = []
    self._rm_browser_tmp_dir()

  def _tear_down_browser(self, is_shutdown: bool) -> None:
    if not self.browser_session.is_last_run(self):
      logging.debug("Skipping browser teardown (not first in session): %s",
                    self)
      return
    with self.measure("browser-tear_down"):
      if self._browser.is_running is False:
        logging.warning("Browser is no longer running (crashed or closed).")
      else:
        if is_shutdown:
          try:
            self._browser.quit(self._runner)  # pytype: disable=wrong-arg-types
          except Exception as e:  # pylint: disable=broad-except
            logging.warning("Error quitting browser: %s", e)
            return
        with self._exceptions.capture("Quit browser"):
          self._browser.quit(self._runner)  # pytype: disable=wrong-arg-types

  def _tear_down_probe_contexts(self,
                                probe_contexts: List[ProbeContext]) -> None:
    assert self._state == RunState.DONE
    assert probe_contexts, "Expected non-empty probe_contexts list."
    logging.debug("PROBE SCOPE TEARDOWN")
    for probe_context in reversed(probe_contexts):
      with self.exceptions.capture(f"Probe {probe_context.name} teardown"):
        assert probe_context.run == self
        probe_results: ProbeResult = probe_context.tear_down()  # pytype: disable=wrong-arg-types
        probe = probe_context.probe
        if probe_results.is_empty:
          logging.warning("Probe did not extract any data. probe=%s run=%s",
                          probe, self)
        self._probe_results[probe] = probe_results

  def _rm_browser_tmp_dir(self) -> None:
    if not self._browser_tmp_dir:
      return
    self.browser_platform.rm(self._browser_tmp_dir, dir=True)

  def log_results(self) -> None:
    for probe in self.probes:
      probe.log_run_result(self)
