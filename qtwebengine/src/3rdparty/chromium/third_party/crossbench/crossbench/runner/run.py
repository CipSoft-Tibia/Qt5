# Copyright 2023 The Chromium Authors
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

from __future__ import annotations

import datetime as dt
import enum
import logging
from typing import TYPE_CHECKING, Optional

from crossbench import compat
from crossbench import path as pth
from crossbench.exception import Annotator, TInfoStack
from crossbench.helper import ChangeCWD, Durations, Spinner
from crossbench.helper.state import State, StateMachine
from crossbench.probes.probe_context import ProbeContext
from crossbench.probes.results import ProbeResultDict
from crossbench.runner.actions import Actions
from crossbench.runner.probe_context_manager import ProbeContextManager
from crossbench.runner.result_origin import ResultOrigin
from crossbench.runner.timing import Timing

if TYPE_CHECKING:
  from selenium.webdriver.common.options import ArgOptions

  from typing import Iterable

  from crossbench.benchmarks.base import Benchmark
  from crossbench.browsers.browser import Browser
  from crossbench.browsers.secrets import SecretType
  from crossbench.env import HostEnvironment
  from crossbench.probes.probe import Probe
  from crossbench.runner.groups import BrowserSessionRunGroup
  from crossbench.runner.runner import Runner
  from crossbench.stories.story import Story
  from crossbench.types import JsonDict


@enum.unique
class Temperature(compat.StrEnumWithHelp):
  COLD = ("cold", "first run")
  WARM = ("warm", "second run")
  HOT = ("hot", "third run")


class Run(ResultOrigin):

  def __init__(self,
               runner: Runner,
               browser_session: BrowserSessionRunGroup,
               story: Story,
               repetition: int,
               is_warmup: bool,
               temperature: str,
               index: int,
               name: Optional[str] = None,
               timeout: dt.timedelta = dt.timedelta(),
               throw: bool = False):
    self._state = StateMachine(State.INITIAL)
    self._runner = runner
    self._browser_session = browser_session
    self._browser: Browser = browser_session.browser
    browser_session.append(self)
    self._story = story
    assert repetition >= 0
    self._repetition = repetition
    self._is_warmup = is_warmup
    assert temperature, "Missing cache-temperature value."
    self._temperature = temperature
    assert index >= 0
    self._index = index
    self._name = name
    self._out_dir = self._get_out_dir(browser_session.root_dir).absolute()
    self._probe_results = ProbeResultDict(self._out_dir)
    self._durations = Durations()
    self._start_datetime = dt.datetime.utcfromtimestamp(0)
    self._timeout = timeout
    self._exceptions = Annotator(throw)
    self._browser_tmp_dir: Optional[pth.RemotePath] = None
    self._probe_context_manager = ProbeRunContextManager(
        self, self._probe_results)

  def __str__(self) -> str:
    return f"Run({self.name}, {self._state}, {self.browser})"

  def _get_out_dir(self, root_dir: pth.LocalPath) -> pth.LocalPath:
    return (root_dir / pth.safe_filename(self.browser.unique_name) / "stories" /
            pth.safe_filename(self.story.name) / str(self.repetition_name) /
            str(self._temperature))

  @property
  def group_dir(self) -> pth.LocalPath:
    return self.out_dir.parent

  def actions(self,
              name: str,
              verbose: bool = False,
              measure: bool = True) -> Actions:
    return Actions(name, self, verbose=verbose, measure=measure)

  @property
  def info_stack(self) -> TInfoStack:
    return (
        f"Run({self.name})",
        (f"browser={self.browser.type_name} label={self.browser.label} "
         f"binary={self.browser.path}"),
        f"story={self.story}",
        f"repetition={self.repetition_name}",
    )

  def details_json(self) -> JsonDict:
    return {
        "cwd": str(self.out_dir),
        "name": self.name,
        "story": self.story.details_json(),
        "browser": self.get_browser_details_json(),
        "run": {
            "name": self.name,
            "index": self.index,
            "repetition": self.repetition,
            "temperature": self.temperature,
            "isWarmup": self.is_warmup,
        },
        "session": {
            "index": self.browser_session.index,
            "cwd": str(self.browser_session.path)
        },
        "probes": self.results.to_json(),
        "timing": {
            "startDateTime": str(self.start_datetime),
            "duration": self.story.duration.total_seconds(),
            "durations": self.durations.to_json(),
            "timeout": self.timeout.total_seconds(),
            "global": self.timing.to_json(),
        },
        "success": self.is_success,
        "errors": self.exceptions.error_messages()
    }

  @property
  def temperature(self) -> str:
    return self._temperature

  @property
  def timing(self) -> Timing:
    return self.runner.timing

  @property
  def durations(self) -> Durations:
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
  def repetition_name(self) -> str:
    if self.is_warmup:
      return f"warmup_{self.repetition}"
    return str(self.repetition)

  @property
  def repetition(self) -> int:
    return self._repetition

  @property
  def is_warmup(self) -> bool:
    return self._is_warmup

  @property
  def index(self) -> int:
    return self._index

  @property
  def runner(self) -> Runner:
    return self._runner

  @property
  def benchmark(self) -> Benchmark:
    return self._runner.benchmark

  @property
  def browser_session(self) -> BrowserSessionRunGroup:
    return self._browser_session

  @property
  def browser(self) -> Browser:
    return self._browser

  @property
  def environment(self) -> HostEnvironment:
    # TODO: replace with custom BrowserEnvironment
    return self.runner.env

  @property
  def out_dir(self) -> pth.LocalPath:
    """A local directory where all result files are gathered.
    Results from browsers on remote platforms are transferred to this dir
    as well."""
    return self._out_dir

  @property
  def browser_tmp_dir(self) -> pth.RemotePath:
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
  def exceptions(self) -> Annotator:
    return self._exceptions

  @property
  def is_success(self) -> bool:
    return self._exceptions.is_success

  @property
  def session(self) -> BrowserSessionRunGroup:
    return self._browser_session

  def get_browser_details_json(self) -> JsonDict:
    details_json = self.browser.details_json()
    self.session.add_flag_details(details_json)
    return details_json

  def get_local_probe_result_path(self, probe: Probe) -> pth.LocalPath:
    file = self._out_dir / probe.result_path_name
    assert not file.exists(), f"Probe results file exists already. file={file}"
    return file

  def validate_env(self, env: HostEnvironment) -> None:
    """Called before starting a browser / browser session to perform
    a pre-run checklist."""

  def setup(self, is_dry_run: bool) -> None:
    self._state.transition(State.INITIAL, to=State.SETUP)
    self._setup_dirs()
    self._cool_down(is_dry_run)
    with ChangeCWD(self._out_dir), self.exception_info(*self.info_stack):
      self._probe_context_manager.setup(self.probes, is_dry_run)
    self._log_setup()

  def do_logins(self, logins: Iterable[SecretType]) -> None:
    for secret_type in logins:
      if secret_type not in self.browser.secrets:
        raise ValueError(f"Requested {secret_type.name} secret not provided")
      self.browser.secrets[secret_type].login(self)

  def setup_selenium_options(self, options: ArgOptions):
    # TODO: move explicitly to session.
    self._probe_context_manager.setup_selenium_options(options)

  def _setup_dirs(self) -> None:
    self._start_datetime = dt.datetime.now()
    logging.debug("Creating Run(%s) out dir: %s", self, self._out_dir)
    self._out_dir.mkdir(parents=True, exist_ok=True)
    self._create_session_dir()

  def _create_session_dir(self) -> None:
    session_run_dir = self._out_dir / "session"
    assert not session_run_dir.exists(), (
        f"Cannot setup session dir twice: {session_run_dir}")
    if not self.runner.create_symlinks:
      logging.debug("Symlink disabled by command line option")
      return
    if self.runner_platform.is_win:
      logging.debug("Skipping session_dir symlink on windows.")
      return
    # Source: BROWSER / "stories" / STORY / REPETITION / CACHE_TEMP / "session"
    # Target: BROWSER / "sessions" / SESSION
    relative_session_dir = (
        pth.LocalPath("../../../..") /
        self.browser_session.path.relative_to(self.out_dir.parents[3]))
    session_run_dir.symlink_to(relative_session_dir)

  def _log_setup(self) -> None:
    logging.debug("SETUP")
    logging.info(
        "PROBES: %s",
        ", ".join(probe.NAME for probe in self.probes if not probe.is_internal))
    logging.debug("PROBES ALL: %s",
                  ", ".join(probe.NAME for probe in self.probes))
    self.story.log_run_details(self)
    logging.info("RUN DIR: %s", self._out_dir)
    logging.debug("CWD %s", self._out_dir)

  def _cool_down(self, is_dry_run: bool) -> None:
    if is_dry_run:
      return
    with self.measure("runner-cooldown"):
      self._runner.wait(self._runner.timing.cool_down_time, absolute_time=True)
      self._runner.cool_down()

  def run(self, is_dry_run: bool) -> None:
    self._state.transition(State.SETUP, to=State.READY)
    self._start_datetime = dt.datetime.now()
    with ChangeCWD(self._out_dir), self.exception_info(*self.info_stack):
      assert self._probe_context_manager.is_ready
      try:
        self._run(is_dry_run)
      except Exception as e:  # pylint: disable=broad-except
        self._exceptions.append(e)
      finally:
        self.teardown(is_dry_run)

  def _run(self, is_dry_run: bool) -> None:
    self._state.transition(State.READY, to=State.RUN)
    self.browser.splash_screen.run(self)
    with self._probe_context_manager.open(is_dry_run):
      logging.info("RUNNING STORY")
      self._state.expect(State.RUN)
      try:
        with self.measure("run"), Spinner(), self.exceptions.capture():
          if not is_dry_run:
            self._run_story()
      except TimeoutError as e:
        # Handle TimeoutError earlier since they might be caused by
        # throttled down non-foreground browser.
        self._exceptions.append(e)
      if self.is_success:
        with self.exceptions.capture():
          self.environment.check_browser_focused(self.browser)

  def _run_story(self) -> None:
    self._run_story_setup()
    self._story.run(self)
    self._run_story_teardown()

  def _run_story_setup(self) -> None:
    with self.measure("story-setup"):
      self._story.setup(self)
    self._probe_context_manager.start_story()

  def _run_story_teardown(self) -> None:
    self._probe_context_manager.stop_story()
    with self.measure("story-tear-down"):
      self._story.teardown(self)

  def teardown(self, is_dry_run: bool) -> None:
    self._state.transition(State.RUN, to=State.DONE)
    self._teardown_browser(is_dry_run)
    self._probe_context_manager.teardown(is_dry_run)
    if not is_dry_run:
      self._rm_browser_tmp_dir()

  def _teardown_browser(self, is_dry_run: bool) -> None:
    if is_dry_run:
      return
    if not self.browser_session.is_last_run(self):
      logging.debug("Skipping browser teardown (not last in session): %s", self)
      return
    if self._browser.is_running is False:
      logging.warning("Browser is no longer running (crashed or closed).")
      return
    with self.measure("browser-teardown"), self._exceptions.capture(
        "Quit browser"):
      try:
        self._browser.quit(self._runner)  # pytype: disable=wrong-arg-types
      except Exception as e:  # pylint: disable=broad-except
        logging.warning("Error quitting browser: %s", e)
        return

  def _rm_browser_tmp_dir(self) -> None:
    if not self._browser_tmp_dir:
      return
    self.browser_platform.rm(self._browser_tmp_dir, dir=True)

  def log_results(self) -> None:
    for probe in self.probes:
      probe.log_run_result(self)


class ProbeRunContextManager(ProbeContextManager[Run, ProbeContext]):

  def __init__(self, run: Run, probe_results: ProbeResultDict):
    super().__init__(run, probe_results)

  def get_probe_context(self, probe: Probe) -> Optional[ProbeContext]:
    return probe.get_context(self._origin)

  def setup_selenium_options(self, options: ArgOptions):
    for probe_context in self._probe_contexts:
      probe_context.setup_selenium_options(options)

  def start_story(self) -> None:
    with self.measure("probes-start_story_run"):
      for probe_context in self._probe_contexts:
        with self._origin.exception_handler(
            f"Probe {probe_context.name} start_story_run"):
          probe_context.start_story_run()

  def stop_story(self) -> None:
    with self.measure("probes-stop_story_run"):
      for probe_context in self._probe_contexts:
        with self._origin.exception_handler(
            f"Probe {probe_context.name} stop_story_run"):
          probe_context.stop_story_run()
