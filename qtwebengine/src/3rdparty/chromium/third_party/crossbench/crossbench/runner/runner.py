# Copyright 2023 The Chromium Authors
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

from __future__ import annotations

import argparse
import datetime as dt
import enum
import inspect
import logging
from typing import (TYPE_CHECKING, Any, Dict, Iterable, List, Optional,
                    Sequence, Set, Tuple, Type, Union)

from crossbench import cli_helper, compat, exception, helper
from crossbench import path as pth
from crossbench import plt
from crossbench.benchmarks.base import BenchmarkProbeMixin
from crossbench.env import (HostEnvironment, HostEnvironmentConfig,
                            ValidationMode)
from crossbench.helper.state import BaseState, StateMachine
from crossbench.probes import all as all_probes
from crossbench.probes.internal import ResultsSummaryProbe
from crossbench.probes.probe import Probe, ProbeIncompatibleBrowser
from crossbench.probes.trace_processor.trace_processor import \
    TraceProcessorProbe
from crossbench.runner.groups import (BrowserSessionRunGroup, BrowsersRunGroup,
                                      CacheTemperatureRunGroup,
                                      RepetitionsRunGroup, RunThreadGroup,
                                      StoriesRunGroup)
from crossbench.runner.run import Run
from crossbench.runner.timing import Timing

if TYPE_CHECKING:
  from crossbench.benchmarks.base import Benchmark
  from crossbench.browsers.browser import Browser
  from crossbench.stories.story import Story



class RunnerException(exception.MultiException):
  pass


@enum.unique
class ThreadMode(compat.StrEnumWithHelp):
  NONE = ("none", (
      "Execute all browser-sessions sequentially, default. "
      "Low interference risk, use for worry-free time-critical measurements."))
  PLATFORM = ("platform", (
      "Execute browser-sessions from each platform in parallel threads. "
      "Might cause some interference with probes that do heavy "
      "post-processing."))
  BROWSER = ("browser", (
      "Execute browser-sessions from each browser in parallel thread. "
      "High interference risk, don't use for time-critical measurements."))
  SESSION = ("session", (
      "Execute run from each browser-session in a parallel thread. "
      "High interference risk, don't use for time-critical measurements."))

  def group(self, runs: List[Run]) -> List[RunThreadGroup]:
    if self == ThreadMode.NONE:
      return [RunThreadGroup(runs)]
    groups: Dict[Any, List[Run]] = {}
    if self == ThreadMode.SESSION:
      groups = helper.group_by(
          runs, lambda run: run.browser_session, sort_key=None)
    elif self == ThreadMode.PLATFORM:
      groups = helper.group_by(
          runs, lambda run: run.browser_platform, sort_key=None)
    elif self == ThreadMode.BROWSER:
      groups = helper.group_by(runs, lambda run: run.browser, sort_key=None)
    else:
      raise ValueError(f"Unexpected thread mode: {self}")
    return [
        RunThreadGroup(runs, index=index)
        for index, runs in enumerate(groups.values())
    ]


@enum.unique
class RunnerState(BaseState):
  INITIAL = enum.auto()
  SETUP = enum.auto()
  RUNNING = enum.auto()
  TEARDOWN = enum.auto()

class Runner:

  @classmethod
  def get_out_dir(cls,
                  cwd: pth.LocalPath,
                  suffix: str = "",
                  test: bool = False) -> pth.LocalPath:
    if test:
      return cwd / "results" / "test"
    if suffix:
      suffix = "_" + suffix
    return (cwd / "results" /
            f"{dt.datetime.now().strftime('%Y-%m-%d_%H%M%S')}{suffix}")

  @classmethod
  def add_cli_parser(
      cls, benchmark_cls: Type[Benchmark],
      parser: argparse.ArgumentParser) -> argparse.ArgumentParser:
    parser.add_argument(
        "--repetitions",
        "--repeat",
        "--invocations",
        "-r",
        default=1,
        type=cli_helper.parse_positive_int,
        help=("Number of times each benchmark story is repeated. "
              "Defaults to 1. "
              "Metrics are aggregated over multiple repetitions"))
    parser.add_argument(
        "--warmup-repetitions",
        "--warmups",
        default=0,
        type=cli_helper.parse_positive_zero_int,
        help=("Number of times each benchmark story is repeated for warmup. "
              "Defaults to 0. "
              "Metrics for warmup-repetitions are discarded."))
    parser.add_argument(
        "--cache-temperatures",
        default=["default"],
        const=["cold", "warm", "hot"],
        action="store_const",
        help=("Repeat each run with different cache temperatures without "
              "closing the browser in between."))

    parser.add_argument(
        "--thread-mode",
        "--parallel",
        default=ThreadMode.NONE,
        type=ThreadMode,
        help=("Change how Runs are executed.\n" +
              ThreadMode.help_text(indent=2)))

    out_dir_group = parser.add_argument_group("Output Directory Options")
    out_dir_group.add_argument(
        "--no-symlinks",
        "--nosymlinks",
        dest="create_symlinks",
        action="store_false",
        default=True,
        help="Do not create symlinks in the output directory.")

    out_dir_xor_group = out_dir_group.add_mutually_exclusive_group()
    out_dir_xor_group.add_argument(
        "--out-dir",
        "--output-directory",
        "-o",
        type=pth.LocalPath,
        help=("Results will be stored in this directory. "
              "Defaults to results/${DATE}_${LABEL}"))
    out_dir_xor_group.add_argument(
        "--label",
        "--name",
        type=cli_helper.parse_non_empty_str,
        default=benchmark_cls.NAME,
        help=("Add a name to the default output directory. "
              "Defaults to the benchmark name"))
    return parser

  @classmethod
  def kwargs_from_cli(cls, args: argparse.Namespace) -> Dict[str, Any]:
    if args.out_dir:
      out_dir = args.out_dir
    else:
      label = args.label
      assert label
      root_dir = pth.LocalPath(__file__).parents[2]
      out_dir = cls.get_out_dir(root_dir, label)
    return {
        "out_dir": out_dir,
        "browsers": args.browser,
        "repetitions": args.repetitions,
        "warmup_repetitions": args.warmup_repetitions,
        "cache_temperatures": args.cache_temperatures,
        "thread_mode": args.thread_mode,
        "throw": args.throw,
        "create_symlinks": args.create_symlinks,
    }

  def __init__(self,
               out_dir: pth.LocalPath,
               browsers: Sequence[Browser],
               benchmark: Benchmark,
               additional_probes: Iterable[Probe] = (),
               platform: plt.Platform = plt.PLATFORM,
               env_config: Optional[HostEnvironmentConfig] = None,
               env_validation_mode: ValidationMode = ValidationMode.THROW,
               repetitions: int = 1,
               warmup_repetitions: int = 0,
               cache_temperatures: Iterable[str] = ("default",),
               timing: Timing = Timing(),
               thread_mode: ThreadMode = ThreadMode.NONE,
               throw: bool = False,
               create_symlinks: bool = True):
    self._state = StateMachine(RunnerState.INITIAL)
    self.out_dir = out_dir
    assert not self.out_dir.exists(), f"out_dir={self.out_dir} exists already"
    self.out_dir.mkdir(parents=True)
    self._timing = timing
    self._browsers: Tuple[Browser, ...] = tuple(browsers)
    self._validate_browsers()
    self._benchmark = benchmark
    self._stories = tuple(benchmark.stories)
    self._repetitions = cli_helper.parse_positive_int(repetitions,
                                                      "repetitions")
    self._warmup_repetitions = cli_helper.parse_positive_zero_int(
        warmup_repetitions, "warmup repetitions")
    self._cache_temperatures: Tuple[str, ...] = tuple(cache_temperatures)
    self._probes: List[Probe] = []
    self._default_probes: List[Probe] = []
    # Contains both measure and warmup runs:
    self._all_runs: List[Run] = []
    self._measured_runs: List[Run] = []
    self._thread_mode = thread_mode
    self._exceptions = exception.Annotator(throw)
    self._platform = platform
    self._env = HostEnvironment(
        self,  # pytype: disable=wrong-arg-types
        env_config,
        env_validation_mode)
    self._attach_default_probes(additional_probes)
    self._prepare_benchmark()
    self._cache_temperature_groups: Tuple[CacheTemperatureRunGroup, ...] = ()
    self._repetitions_groups: Tuple[RepetitionsRunGroup, ...] = ()
    self._story_groups: Tuple[StoriesRunGroup, ...] = ()
    self._browser_group: Optional[BrowsersRunGroup] = None
    self._create_symlinks: bool = create_symlinks

  def _prepare_benchmark(self) -> None:
    benchmark_probe_cls: Type[BenchmarkProbeMixin]
    for benchmark_probe_cls in self._benchmark.PROBES:
      assert inspect.isclass(benchmark_probe_cls), (
          f"{self._benchmark}.PROBES must contain classes only, "
          f"but got {type(benchmark_probe_cls)}")
      assert issubclass(
          benchmark_probe_cls,
          Probe), (f"Expected Probe class but got {type(benchmark_probe_cls)}")
      assert issubclass(benchmark_probe_cls, BenchmarkProbeMixin), (
          f"{benchmark_probe_cls} should be BenchmarkProbeMixin "
          f"for {type(self._benchmark)}.PROBES")
      assert benchmark_probe_cls.NAME, (
          f"Expected probe.NAME for {benchmark_probe_cls}")
      self.attach_probe(benchmark_probe_cls(benchmark=self._benchmark))

  def _validate_browsers(self) -> None:
    assert self.browsers, "No browsers provided"
    browser_unique_names = [browser.unique_name for browser in self.browsers]
    cli_helper.parse_unique_sequence(browser_unique_names, "browser names")

  def _attach_default_probes(self, probe_list: Iterable[Probe]) -> None:
    assert len(self._probes) == 0
    assert len(self._default_probes) == 0
    for probe_cls in all_probes.INTERNAL_PROBES:
      default_probe: Probe = probe_cls()  # pytype: disable=not-instantiable
      self.attach_probe(default_probe)
      self._default_probes.append(default_probe)
    for index, probe in enumerate(probe_list):
      assert (not isinstance(probe, TraceProcessorProbe) or index == 0), (
          f"TraceProcessorProbe must be first in the list to be able "
          f"to process other probes data. Found it at index: {index}")
      self.attach_probe(probe)
    # Results probe must be first in the list, and thus last to be processed
    # so all other probes have data by the time we write the results summary.
    assert isinstance(self._probes[0], ResultsSummaryProbe)

  def attach_probe(self,
                   probe: Probe,
                   matching_browser_only: bool = False) -> Probe:
    if probe in self._probes:
      raise ValueError(f"Cannot add the same probe twice: {probe.NAME}")
    probe_was_used = False
    for browser in self.browsers:
      try:
        probe.validate_browser(self.env, browser)
        browser.attach_probe(probe)
        probe_was_used = True
      except ProbeIncompatibleBrowser as e:
        if matching_browser_only:
          logging.error("Skipping incompatible probe=%s for browser=%s:",
                        probe.name, browser.unique_name)
          logging.error("    %s", e)
          continue
        raise
    if probe_was_used:
      self._probes.append(probe)
    return probe

  @property
  def timing(self) -> Timing:
    return self._timing

  @property
  def cache_temperatures(self) -> Tuple[str, ...]:
    return self._cache_temperatures

  @property
  def browsers(self) -> Tuple[Browser, ...]:
    return self._browsers

  @property
  def stories(self) -> Tuple[Story, ...]:
    return self._stories

  @property
  def probes(self) -> Iterable[Probe]:
    return iter(self._probes)

  @property
  def default_probes(self) -> Iterable[Probe]:
    return iter(self._default_probes)

  @property
  def benchmark(self) -> Benchmark:
    return self._benchmark

  @property
  def repetitions(self) -> int:
    return self._repetitions

  @property
  def warmup_repetitions(self) -> int:
    return self._warmup_repetitions

  @property
  def create_symlinks(self) -> bool:
    return self._create_symlinks

  @property
  def exceptions(self) -> exception.Annotator:
    return self._exceptions

  @property
  def is_success(self) -> bool:
    return len(self._measured_runs) > 0 and self._exceptions.is_success

  @property
  def platform(self) -> plt.Platform:
    return self._platform

  @property
  def env(self) -> HostEnvironment:
    return self._env

  @property
  def platforms(self) -> Set[plt.Platform]:
    return set(browser.platform for browser in self.browsers)

  @property
  def all_runs(self) -> Tuple[Run, ...]:
    return tuple(self._all_runs)

  @property
  def runs(self) -> Tuple[Run, ...]:
    return tuple(self._measured_runs)

  @property
  def cache_temperature_groups(self) -> Tuple[CacheTemperatureRunGroup, ...]:
    assert self._cache_temperature_groups, (
        f"No CacheTemperatureRunGroup in {self}")
    return self._cache_temperature_groups

  @property
  def repetitions_groups(self) -> Tuple[RepetitionsRunGroup, ...]:
    assert self._repetitions_groups, f"No RepetitionsRunGroup in {self}"
    return self._repetitions_groups

  @property
  def story_groups(self) -> Tuple[StoriesRunGroup, ...]:
    assert self._story_groups, f"No StoriesRunGroup in {self}"
    return self._story_groups

  @property
  def browser_group(self) -> BrowsersRunGroup:
    assert self._browser_group, f"No BrowsersRunGroup in {self}"
    return self._browser_group

  @property
  def has_browser_group(self) -> bool:
    return self._browser_group is not None

  def wait(self,
           time: Union[int, float, dt.timedelta],
           absolute_time: bool = False) -> None:
    if not absolute_time:
      delta = self.timing.timedelta(time)
    else:
      if isinstance(time, (int, float)):
        delta = dt.timedelta(seconds=time)
      else:
        delta = time
    self._platform.sleep(delta)

  def run(self, is_dry_run: bool = False) -> None:
    self._state.expect(RunnerState.INITIAL)
    with helper.SystemSleepPreventer():
      with self._exceptions.annotate("Preparing"):
        self._setup()
      with self._exceptions.annotate("Running"):
        self._run(is_dry_run)

    if self._exceptions.throw:
      # Ensure that we bail out on the first exception.
      self.assert_successful_sessions_and_runs()
    if not is_dry_run:
      self._teardown()
    self.assert_successful_sessions_and_runs()

  def _setup(self) -> None:
    self._state.transition(RunnerState.INITIAL, to=RunnerState.SETUP)
    logging.info("-" * 80)
    logging.info("SETUP")
    logging.info("-" * 80)
    assert self.repetitions > 0, (
        f"Invalid repetitions count: {self.repetitions}")
    assert self.browsers, "No browsers provided: self.browsers is empty"
    assert self.stories, "No stories provided: self.stories is empty"
    self._setup_browsers()
    self._exceptions.assert_success()
    with self._exceptions.annotate("Preparing Runs"):
      self._all_runs = list(self.get_runs())
      assert self._all_runs, f"{type(self)}.get_runs() produced no runs"
      logging.info("DISCOVERED %d RUN(S)", len(self._all_runs))
      self._measured_runs = [run for run in self._all_runs if not run.is_warmup]
    with self._exceptions.capture("Preparing Environment"):
      self._env.setup()
    with self._exceptions.annotate(
        f"Preparing Benchmark: {self._benchmark.NAME}"):
      self._benchmark.setup(self)  # pytype:  disable=wrong-arg-types

  def _setup_browsers(self) -> None:
    logging.info("PREPARING %d BROWSER(S)", len(self.browsers))
    for browser in self.browsers:
      with self._exceptions.capture(
          f"Preparing browser type={browser.type_name} "
          f"unique_name={browser.unique_name}"):
        self._setup_browser(browser)

  def _setup_browser(self, browser: Browser) -> None:
    browser.setup_binary(self)  # pytype: disable=wrong-arg-types
    for probe in browser.probes:
      assert probe in self._probes, (
          f"Browser {browser} probe {probe} not in Runner.probes. "
          "Use Runner.attach_probe()")

  def has_any_live_network(self) -> bool:
    for browser in self.browsers:
      if browser.network.is_live:
        return True
    return False

  def get_runs(self) -> Iterable[Run]:
    index = 0
    session_index = 0
    throw = self._exceptions.throw
    total_repetitions = self.repetitions + self.warmup_repetitions
    for repetition in range(total_repetitions):
      is_warmup: bool = repetition < self.warmup_repetitions
      for story in self.stories:
        for browser in self.browsers:
          # TODO: implement browser-session start/stop
          browser_session = BrowserSessionRunGroup(self, browser, session_index,
                                                   self.out_dir, throw)
          session_index += 1
          for t_index, temperature in enumerate(self.cache_temperatures):
            name_parts = [f"story={story.name}"]
            if total_repetitions > 1:
              name_parts.append(f"repetition={repetition}")
            if len(self.cache_temperatures) > 1:
              name_parts.append(f"cache={temperature}")
            name_parts.append(f"index={index}")
            yield self.create_run(
                browser_session,
                story,
                repetition,
                is_warmup,
                f"{t_index}_{temperature}",
                index,
                name=", ".join(name_parts),
                timeout=self.timing.run_timeout,
                throw=throw)
            index += 1
          browser_session.set_ready()

  def create_run(self, browser_session: BrowserSessionRunGroup, story: Story,
                 repetition: int, is_warmup: bool, temperature: str, index: int,
                 name: str, timeout: dt.timedelta, throw: bool) -> Run:
    return Run(self, browser_session, story, repetition, is_warmup, temperature,
               index, name, timeout, throw)

  def assert_successful_sessions_and_runs(self) -> None:
    failed_runs = list(run for run in self.runs if not run.is_success)
    self._exceptions.assert_success(
        f"Runs Failed: {len(failed_runs)}/{len(tuple(self.runs))} runs failed.",
        RunnerException)

  def _get_thread_groups(self) -> List[RunThreadGroup]:
    # Also include warmup runs here.
    return self._thread_mode.group(self._all_runs)

  def _run(self, is_dry_run: bool = False) -> None:
    self._state.transition(RunnerState.SETUP, to=RunnerState.RUNNING)
    thread_groups: List[RunThreadGroup] = []
    with self._exceptions.info("Creating thread groups for all Runs"):
      thread_groups = self._get_thread_groups()

    group_count = len(thread_groups)
    if group_count == 1:
      self._run_single_threaded(thread_groups[0])
      return

    with self._exceptions.annotate(f"Starting {group_count} thread groups."):
      for thread_group in thread_groups:
        thread_group.is_dry_run = is_dry_run
        thread_group.start()
    with self._exceptions.annotate(
        "Waiting for all thread groups to complete."):
      for thread_group in thread_groups:
        thread_group.join()

  def _run_single_threaded(self, thread_group: RunThreadGroup) -> None:
    # Special case single thread groups
    with self._exceptions.annotate("Running single thread group"):
      thread_group.run()

  def _teardown(self) -> None:
    self._state.transition(RunnerState.RUNNING, to=RunnerState.TEARDOWN)
    logging.info("=" * 80)
    logging.info("RUNS COMPLETED")
    logging.info("-" * 80)
    logging.info("MERGING PROBE DATA")

    throw = self._exceptions.throw

    logging.debug("MERGING PROBE DATA: cache temperatures")
    self._cache_temperature_groups = CacheTemperatureRunGroup.groups(
        self._measured_runs, throw)
    for cache_temp_group in self._cache_temperature_groups:
      cache_temp_group.merge(self)
      self._exceptions.extend(cache_temp_group.exceptions, is_nested=True)

    logging.debug("MERGING PROBE DATA: repetitions")
    self._repetitions_groups = RepetitionsRunGroup.groups(
        self._cache_temperature_groups, throw)
    for repetition_group in self._repetitions_groups:
      repetition_group.merge(self)
      self._exceptions.extend(repetition_group.exceptions, is_nested=True)

    logging.debug("MERGING PROBE DATA: stories")
    self._story_groups = StoriesRunGroup.groups(self._repetitions_groups, throw)
    for story_group in self._story_groups:
      story_group.merge(self)
      self._exceptions.extend(story_group.exceptions, is_nested=True)

    logging.debug("MERGING PROBE DATA: browsers")
    self._browser_group = BrowsersRunGroup(self._story_groups, throw)
    self._browser_group.merge(self)
    self._exceptions.extend(self._browser_group.exceptions, is_nested=True)

  def cool_down(self) -> None:
    # Cool down between runs
    if not self._platform.is_thermal_throttled():
      return
    logging.info("COOLDOWN")
    for _ in helper.WaitRange(1, 100).wait_with_backoff(self._platform):
      if not self._platform.is_thermal_throttled():
        break
      logging.info("COOLDOWN: still hot, waiting some more")
