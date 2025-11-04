# Copyright 2022 The Chromium Authors
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

from __future__ import annotations

import abc
import atexit
import enum
import io
import json
import logging
import multiprocessing
import signal
import subprocess
import time
from functools import cached_property
from typing import (TYPE_CHECKING, Dict, Final, Iterable, List, Optional,
                    Sequence, Tuple, cast)

from crossbench import helper
from crossbench import path as pth
from crossbench import plt
from crossbench.browsers.attributes import BrowserAttributes
from crossbench.browsers.chrome.version import ChromeVersion
from crossbench.browsers.chromium.chromium import Chromium
from crossbench.compat import StrEnumWithHelp
from crossbench.plt.base import ListCmdArgs
from crossbench.probes.probe import (Probe, ProbeConfigParser, ProbeContext,
                                     ProbeIncompatibleBrowser, ProbeKeyT,
                                     ResultLocation)
from crossbench.probes.results import ProbeResult
from crossbench.probes.v8.log import V8LogProbe

if TYPE_CHECKING:
  from crossbench.browsers.browser import Browser
  from crossbench.env import HostEnvironment
  from crossbench.runner.groups import BrowsersRunGroup
  from crossbench.runner.run import Run


@enum.unique
class CleanupMode(StrEnumWithHelp):

  @classmethod
  def _missing_(cls, value) -> Optional[CleanupMode]:
    if value is True:
      return CleanupMode.ALWAYS
    if value is False:
      return CleanupMode.NEVER
    return super()._missing_(value)

  ALWAYS = ("always", "Always clean up temp files")
  AUTO = ("auto", "Best-guess auto-cleanup")
  NEVER = ("never", "Always clean up temp files")


@enum.unique
class TargetMode(StrEnumWithHelp):
  RENDERER_MAIN_ONLY = ("renderer_main_only",
                        "Profile Renderer Main thread only")
  RENDERER_PROCESS_ONLY = ("renderer_process_only",
                           "Profile Renderer process only")
  BROWSER_APP_ONLY = ("browser_app_only",
                      "Profile all processes of the Browser App only")
  SYSTEM_WIDE = ("system_wide", "Run system-wide profiling")


@enum.unique
class CallGraphMode(StrEnumWithHelp):
  # Refer to the documentation below for more details and comparison
  # between these options:
  # https://android.googlesource.com/platform/system/extras/+/master/simpleperf/doc/README.md.
  NO_CALL_GRAPH = ("no_call_graph", "Do not record a call graph")
  DWARF = ("dwarf", "Run DWARF-based unwinding unwinding")
  FRAME_POINTER = ("frame_pointer", "Run frame pointer unwinding")


V8_INTERPRETED_FRAMES_FLAG = "--interpreted-frames-native-stack"

RENDERER_CMD_PATH: Final[pth.LocalPath] = pth.LocalPath(
    __file__).parent / "linux-perf-chrome-renderer-cmd.sh"

class ProfilingProbe(Probe):
  """
  General-purpose sampling profiling probe.

  Implementation:
  - Uses linux-perf on linux platforms (per browser/renderer process)
  - Uses xctrace on MacOS (currently only system-wide)
  - Uses simpleperf on Android (renderer-only, browser-only, or system-wide)

  For linux-based Chromium browsers it also injects JS stack samples with names
  from V8. For Googlers it additionally can auto-upload symbolized profiles to
  pprof.
  """
  NAME = "profiling"
  RESULT_LOCATION = ResultLocation.BROWSER
  IS_GENERAL_PURPOSE = True

  @classmethod
  def config_parser(cls) -> ProbeConfigParser:
    parser = super().config_parser()
    parser.add_argument(
        "js",
        type=bool,
        default=True,
        help=("Chrome-on-Linux-only: expose JS function names to the native "
              "profiler"))
    parser.add_argument(
        "browser_process",
        type=bool,
        default=False,
        help=("Chrome-on-Linux-only: also profile the browser process, "
              "(as opposed to only renderer processes)"))
    parser.add_argument(
        "spare_renderer_process",
        type=bool,
        default=False,
        help=("Chrome-only: Enable/Disable spare renderer processes via \n"
              "--enable-/--disable-features=SpareRendererForSitePerProcess.\n"
              "Spare renderers are disabled by default when profiling "
              "for fewer uninteresting processes."))
    parser.add_argument(
        "v8_interpreted_frames",
        type=bool,
        default=True,
        help=(
            f"Chrome-only: Sets the {V8_INTERPRETED_FRAMES_FLAG} flag for "
            "V8, which exposes interpreted frames as native frames. "
            "Note that this comes at an additional performance and memory cost."
        ))
    parser.add_argument(
        "pprof",
        type=bool,
        default=True,
        help="linux-only: process collected samples with pprof.")
    parser.add_argument(
        "cleanup",
        type=CleanupMode,
        default=CleanupMode.AUTO,
        help="Automatically clean up any temp files "
        "(perf.data.jitted and temporary .so files on linux "
        "cleaned up automatically if pprof is set to True)")
    # Android/simpleperf-specific arguments.
    parser.add_argument(
        "target",
        type=TargetMode,
        default=TargetMode.BROWSER_APP_ONLY,
        help=("Chrome-on-Android-only: "
              "Profile either Renderer main/process only, "
              "or all processes of the Browser App, or system-wide. "
              "If Renderer main/process profiling is selected, "
              "profiling begins **after** browser has started "
              "and the benchmark story has been setup."))
    parser.add_argument(
        "pin_renderer_main_core",
        type=int,
        default=None,
        help=("Chrome-on-Android-only: "
              "Whether to pin the renderer main thread to a given core"))
    parser.add_argument(
        "call_graph_mode",
        type=CallGraphMode,
        default=CallGraphMode.FRAME_POINTER,
        help=("Android-only: Specify whether to record a call graph, "
              "and, if yes, which kind of stack unwinding to run."))
    # Advanced Android/simpleperf-specific arguments.
    # Generally, the defaults should suffice.
    parser.add_argument(
        "frequency",
        type=int,
        default=None,
        help=("Android-only: Event sampling frequency "
              "(record at most `frequency` samples every second). "
              "Please refer to the simpleperf documentation "
              "for `freq` for more details."))
    parser.add_argument(
        "count",
        type=int,
        default=None,
        help=("Android-only: Event sampling period "
              "(record one sample every `count` events). "
              "Please refer to simpleperf documentation for more details."))
    parser.add_argument(
        "cpu",
        type=int,
        is_list=True,
        default=tuple(),
        help=("Android-only: Sample only on the selected cpus, "
              "specified as a list of 0-indexed cpu indices. "
              "Please refer to simpleperf documentation for more details."))
    parser.add_argument(
        "events",
        type=str,
        is_list=True,
        default=tuple(),
        help=("Android-only: Events to record. Please refer to simpleperf "
              "documentation for `-e` for more details."))
    parser.add_argument(
        "grouped_events",
        type=str,
        is_list=True,
        default=tuple(),
        help=("Android-only: Events to record as a single group. "
              "These events are monitored as a group, "
              "and scheduled in and out together. "
              "Please refer to simpleperf documentation for `--group` "
              "for more details."))
    parser.add_argument(
        "add_counters",
        type=str,
        is_list=True,
        default=tuple(),
        help=("Android-only: Add additional event counts in samples. NOTE: If "
              "`add_counter` is used, `--no-inherit` is implicitly set, since "
              "this is required by simpleperf. Please refer to simpleperf "
              "documentation for `--add-counter` and `--no-inherit` for more "
              "details."))
    return parser

  def __init__(self,
               js: bool = True,
               v8_interpreted_frames: bool = True,
               pprof: bool = True,
               cleanup: CleanupMode = CleanupMode.AUTO,
               browser_process: bool = False,
               spare_renderer_process: bool = False,
               target: TargetMode = TargetMode.BROWSER_APP_ONLY,
               pin_renderer_main_core: Optional[int] = None,
               call_graph_mode: CallGraphMode = CallGraphMode.FRAME_POINTER,
               frequency: Optional[int] = None,
               count: Optional[int] = None,
               cpu: Sequence[int] = (),
               events: Sequence[str] = (),
               grouped_events: Sequence[str] = (),
               add_counters: Sequence[str] = ()):
    super().__init__()
    self._sample_js: bool = js
    self._sample_browser_process: bool = browser_process
    self._spare_renderer_process: bool = spare_renderer_process
    self._run_pprof: bool = pprof
    self._cleanup_mode = cleanup
    self._expose_v8_interpreted_frames: bool = v8_interpreted_frames
    if v8_interpreted_frames:
      assert js, "Cannot expose V8 interpreted frames without js profiling."
    self._target: TargetMode = target
    self._pin_renderer_main_core: Optional[int] = pin_renderer_main_core
    self._call_graph_mode: CallGraphMode = call_graph_mode
    self._start_profiling_after_setup: bool = target in (
        TargetMode.RENDERER_MAIN_ONLY,
        TargetMode.RENDERER_PROCESS_ONLY) or pin_renderer_main_core is not None
    self._frequency: Optional[int] = frequency
    self._count: Optional[int] = count
    self._cpu: Tuple[int, ...] = tuple(cpu)
    self._events: Tuple[str, ...] = tuple(events)
    self._grouped_events: Tuple[str, ...] = tuple(grouped_events)
    self._add_counters: Tuple[str, ...] = tuple(add_counters)

  @property
  def key(self) -> ProbeKeyT:
    return super().key + (
        ("js", self._sample_js),
        ("v8_interpreted_frames", self._expose_v8_interpreted_frames),
        ("pprof", self._run_pprof),
        ("cleanup", self._cleanup_mode),
        ("browser_process", self._sample_browser_process),
        ("spare_renderer_process", self._spare_renderer_process),
        ("target", str(self._target)),
        ("pin_renderer_main_core", self._pin_renderer_main_core),
        ("call_graph_mode", str(self._call_graph_mode)),
        ("start_profiling_after_setup", self._start_profiling_after_setup),
        ("frequency", self._frequency),
        ("count", self._count),
        ("cpu", self._cpu),
        ("events", self._events),
        ("grouped_events", self._grouped_events),
        ("add_counters", self._add_counters),
    )

  @property
  def sample_js(self) -> bool:
    return self._sample_js

  @property
  def sample_browser_process(self) -> bool:
    return self._sample_browser_process

  @property
  def run_pprof(self) -> bool:
    return self._run_pprof

  @property
  def cleanup_mode(self) -> CleanupMode:
    return self._cleanup_mode

  @property
  def target(self) -> TargetMode:
    return self._target

  @property
  def pin_renderer_main_core(self) -> Optional[int]:
    return self._pin_renderer_main_core

  @property
  def call_graph_mode(self) -> CallGraphMode:
    return self._call_graph_mode

  @property
  def start_profiling_after_setup(self) -> bool:
    return self._start_profiling_after_setup

  @property
  def frequency(self) -> Optional[int]:
    return self._frequency

  @property
  def count(self) -> Optional[int]:
    return self._count

  @property
  def cpu(self) -> Tuple[int, ...]:
    return self._cpu

  @property
  def events(self) -> Tuple[str, ...]:
    return self._events

  @property
  def grouped_events(self) -> Tuple[str, ...]:
    return self._grouped_events

  @property
  def add_counters(self) -> Tuple[str, ...]:
    return self._add_counters

  def attach(self, browser: Browser) -> None:
    super().attach(browser)
    if browser.platform.is_linux or browser.platform.is_android:
      assert browser.attributes.is_chromium_based, (
          f"Expected Chromium-based browser, found {type(browser)}.")
    if browser.attributes.is_chromium_based:
      chromium = cast(Chromium, browser)
      if not self._spare_renderer_process:
        chromium.features.disable("SpareRendererForSitePerProcess")
      self._attach(chromium)

  def validate_browser(self, env: HostEnvironment, browser: Browser) -> None:
    browser_platform = browser.platform
    if browser_platform.is_linux:
      self._validate_linux(env, browser)
    elif browser_platform.is_macos:
      self._validate_macos(env, browser)
    elif browser_platform.is_android:
      self._validate_android(env, browser)
    else:
      raise ProbeIncompatibleBrowser(self, browser)
    if self.run_pprof:
      self._validate_pprof(env, browser)
    # Check that certain Android-only options are
    # not provided by on other platforms.
    if not browser_platform.is_android:
      assert self._frequency is None, (
          "`frequency` is currently only supported on Android")
      assert self._count is None, (
          "`count` is currently only supported on Android")
      assert not self._cpu, ("`cpu` is currently only supported on Android")
      assert not self._events, (
          "`events` is currently only supported on Android")
      assert not self._grouped_events, (
          "`grouped_events` is currently only supported on Android")
      assert not self._add_counters, (
          "`add_counters` is currently only supported on Android")

  def _validate_linux(self, env: HostEnvironment, browser: Browser) -> None:
    env.check_installed(binaries=["pprof"])
    assert browser.platform.which("perf"), "Please install linux-perf"

  def _validate_macos(self, env: HostEnvironment, browser: Browser) -> None:
    assert browser.platform.which(
        "xctrace"), "Please install Xcode to use xctrace"
    # Only Linux-perf and Android-simpleperf results can be merged
    if env.runner.repetitions > 1:
      env.handle_warning(f"Probe={self.NAME} cannot merge data over multiple "
                         f"repetitions={env.runner.repetitions}.")

  def _assert_is_chrome_with_extension(self, browser: Browser) -> None:
    assert (
        BrowserAttributes.CHROME in browser.attributes and
        browser.major_version >= 124), (
            "For RENDERER_MAIN_ONLY/RENDERER_PROCESS_ONLY profiling, "
            "browser version >= M124 https://crrev.com/c/5374765 is required.")

  def _requires_chrome_with_extension(self) -> bool:
    return self._target in (TargetMode.RENDERER_MAIN_ONLY,
                            TargetMode.RENDERER_PROCESS_ONLY
                           ) or self._pin_renderer_main_core is not None

  def _validate_android(self, env: HostEnvironment, browser: Browser) -> None:
    del env

    if self._requires_chrome_with_extension():
      self._assert_is_chrome_with_extension(browser)

    assert browser.platform.which("simpleperf"), "simpleperf is not available"

  def _validate_pprof(self, env: HostEnvironment, browser: Browser) -> None:
    assert self._run_pprof
    host_platform = browser.platform.host_platform
    self._run_pprof = host_platform.which("gcert") is not None
    if not self.run_pprof:
      logging.warning(
          "Disabled automatic pprof uploading for non-googler machine.")
      return
    if browser.platform.is_macos:
      # Converting xctrace to pprof is not supported on macos
      return
    try:
      if gcertstatus := browser.platform.which("gcertstatus"):
        browser.platform.sh(gcertstatus)
        return
      env.handle_warning("Could not find gcertstatus")
    except plt.SubprocessError:
      env.handle_warning("Please run gcert for generating pprof results")

  def _attach(self, browser: Chromium) -> None:
    if self._sample_js:
      if browser.platform.is_linux:
        browser.js_flags.set("--perf-prof")
      if self._expose_v8_interpreted_frames:
        browser.js_flags.set(V8_INTERPRETED_FRAMES_FLAG)
    if browser.platform.is_linux and browser.platform.is_local:
      assert not browser.platform.is_remote, (
          "Copying renderer command prefix to remote platform is "
          "not implemented yet")
      assert RENDERER_CMD_PATH.is_file(), f"Didn't find {RENDERER_CMD_PATH}"
      browser.flags["--renderer-cmd-prefix"] = str(RENDERER_CMD_PATH)
    # Disable sandbox to write profiling data
    browser.flags.set("--no-sandbox")

  def log_run_result(self, run: Run) -> None:
    self._log_results([run])

  def log_browsers_result(self, group: BrowsersRunGroup) -> None:
    self._log_results(group.runs)

  def _log_results(self, runs: Iterable[Run]) -> None:
    filtered_runs = list(run for run in runs if self in run.results)
    if not filtered_runs:
      return
    logging.info("-" * 80)
    logging.critical("Profiling results:")
    self._log_results_overview(filtered_runs)
    logging.info("- " * 40)
    for i, run in enumerate(filtered_runs):
      self._log_run_result_summary(run, i)

  def _log_results_overview(self, filtered_runs):
    if len(filtered_runs) <= 1:
      return
    if any(run.browser.platform.is_macos for run in filtered_runs):
      logging.info("  *.trace:     'open $FILE'")
    if any(run.browser.platform.is_linux or run.browser.platform.is_android
           for run in filtered_runs):
      logging.info("  *.perf.data: 'perf report -i $FILE'")

  def _log_run_result_summary(self, run: Run, i: int) -> None:
    if self not in run.results:
      return
    urls = run.results[self].url_list
    perf_files = run.results[self].file_list
    if not urls and not perf_files:
      return
    logging.info("Run %d: %s", i + 1, run.name)
    if urls:
      largest_perf_file = perf_files[0]
      logging.critical("    %s", urls[0])
    if not perf_files:
      return
    largest_perf_file = perf_files[0]
    logging.critical("    %s : %s", largest_perf_file,
                     helper.get_file_size(largest_perf_file))
    if len(perf_files) <= 1:
      return
    glob = "*.perf.data"
    if run.browser.platform.is_macos:
      glob = "*.trace"
    logging.info("    %s/%s: %d more files", largest_perf_file.parent, glob,
                 len(perf_files))

  def get_context(self, run: Run) -> ProfilingContext:
    if run.browser_platform.is_linux:
      return LinuxProfilingContext(self, run)
    if run.browser_platform.is_macos:
      return MacOSProfilingContext(self, run)
    if run.browser_platform.is_android:
      return AndroidProfilingContext(self, run)
    raise NotImplementedError("Invalid platform")


class ProfilingContext(ProbeContext[ProfilingProbe], metaclass=abc.ABCMeta):

  def setup_v8_log_path(self) -> None:
    if any(isinstance(probe, V8LogProbe) for probe in self.run.probes):
      return
    # Try to get a bit a cleaner output folder by redirecting v8 logging output
    # to v8.log.
    v8_log_dir = self.result_path.parent / V8LogProbe.NAME / "v8.log"
    self.browser_platform.mkdir(v8_log_dir)
    self.session.extra_js_flags["--logfile"] = str(v8_log_dir)


class MacOSProfilingContext(ProfilingContext):
  _process: Optional[subprocess.Popen]

  def get_default_result_path(self) -> pth.RemotePath:
    return super().get_default_result_path().parent / "profile.trace"

  def start(self) -> None:
    self._process = self.browser_platform.popen("xctrace", "record",
                                                "--template", "Time Profiler",
                                                "--all-processes", "--output",
                                                self.result_path)
    # xctrace takes some time to start up
    time.sleep(3)
    if self._process.poll():
      raise ValueError("Could not start xctrace")
    atexit.register(self.stop_process)

  def stop(self) -> None:
    # Needs to be SIGINT for xctrace, terminate won't work.
    assert self._process
    self._process.send_signal(signal.SIGINT)

  def teardown(self) -> ProbeResult:
    self.stop_process()
    return self.browser_result(file=(self.result_path,))

  def stop_process(self) -> None:
    if self._process:
      logging.info("  Waiting for xctrace profiles (slow)...")
      with helper.Spinner():
        helper.wait_and_kill(self._process, signal=signal.SIGINT, timeout=60)
      self._process = None
    atexit.unregister(self.stop_process)


V8_PERF_RPOF_PATH_FLAG_MIN_VERSION = ChromeVersion((118, 0, 5993, 48))
PERF_DATA_PATTERN = "*.perf.data"
JIT_DUMP_PATTERN = "jit-*.dump"


class LinuxProfilingContext(ProfilingContext):
  TEMP_FILE_PATTERNS = (
      "*.perf.data.jitted",
      "jitted-*.so",
      JIT_DUMP_PATTERN,
  )

  def __init__(self, probe: ProfilingProbe, run: Run) -> None:
    super().__init__(probe, run)
    self._perf_process: Optional[subprocess.Popen] = None

  def get_default_result_path(self) -> pth.RemotePath:
    result_dir = super().get_default_result_path()
    self.browser_platform.mkdir(result_dir)
    return result_dir

  @property
  def has_perf_prof_path(self) -> bool:
    # TODO: replace with full version comparison
    return self.browser.major_version > V8_PERF_RPOF_PATH_FLAG_MIN_VERSION.major

  def setup(self) -> None:
    self.setup_v8_log_path()
    if self.has_perf_prof_path:
      self.session.extra_js_flags["--perf-prof-path"] = str(self.result_path)

  def start(self) -> None:
    if not self.probe.sample_browser_process:
      return
    if self.run.browser.pid is None:
      logging.warning("Cannot sample browser process")
      return
    perf_data_file: pth.RemotePath = self.result_path / "browser.perf.data"
    # TODO: not fully working yet
    self._perf_process = self.browser_platform.popen(
        "perf", "record", "--call-graph=fp", "--freq=max", "--clockid=mono",
        f"--output={perf_data_file}", f"--pid={self.run.browser.pid}")
    if self._perf_process.poll():
      raise ValueError("Could not start linux profiler")
    atexit.register(self.stop_process)

  def stop(self) -> None:
    self.stop_process()

  def stop_process(self) -> None:
    if self._perf_process:
      helper.wait_and_kill(self._perf_process)
      self._perf_process = None

  def teardown(self) -> ProbeResult:
    # Waiting for linux-perf to flush all perf data
    if self.probe.sample_browser_process:
      logging.debug("Waiting for browser process to stop")
      time.sleep(3)
    if self.probe.sample_browser_process:
      logging.info("Browser process did not stop after 3s. "
                   "You might get partial profiles")
    time.sleep(2)

    perf_files: List[pth.RemotePath] = helper.sort_by_file_size(
        list(self.browser_platform.glob(self.result_path, PERF_DATA_PATTERN)),
        self.browser_platform)
    raw_perf_files = perf_files
    urls: List[str] = []
    try:
      if self.probe.sample_js:
        perf_files = self._inject_v8_symbols(self.run, perf_files)
      if self.probe.run_pprof:
        urls = self._export_to_pprof(self.run, perf_files)
    finally:
      self._clean_up_temp_files(self.run)
    if self.probe.run_pprof:
      logging.debug("Profiling results: %s", urls)
      return self.browser_result(url=urls, file=raw_perf_files)
    if self.browser_platform.which("pprof"):
      logging.info("Run pprof over all (or single) perf data files "
                   "for interactive analysis:")
      logging.info("   pprof --http=localhost:1984 %s",
                   " ".join(map(str, perf_files)))
    return self.browser_result(trace=perf_files)

  def _inject_v8_symbols(
      self, run: Run, perf_files: List[pth.RemotePath]) -> List[pth.RemotePath]:
    with run.actions(
        f"Probe {self.probe.name}: "
        f"Injecting V8 symbols into {len(perf_files)} profiles",
        verbose=True), helper.Spinner():
      # Filter out empty files
      perf_files = [
          file for file in perf_files
          if self.browser_platform.file_size(file) > 0
      ]
      if self.browser_platform.is_remote:
        # Use loop, as we cannot easily serialize the remote platform.
        perf_jitted_files = [
            linux_perf_probe_inject_v8_symbols(file, self.browser_platform)
            for file in perf_files
        ]
      else:
        assert self.browser_platform == plt.PLATFORM
        with multiprocessing.Pool() as pool:
          perf_jitted_files = list(
              pool.imap(linux_perf_probe_inject_v8_symbols, perf_files))
      return [file for file in perf_jitted_files if file is not None]

  def _export_to_pprof(self, run: Run,
                       perf_files: List[pth.RemotePath]) -> List[str]:
    assert self.probe.run_pprof
    run_details_json = json.dumps(run.get_browser_details_json())
    with run.actions(
        f"Probe {self.probe.name}: "
        f"exporting {len(perf_files)} profiles to pprof (slow)",
        verbose=True), helper.Spinner():
      self.browser_platform.sh(
          "gcertstatus >&/dev/null || "
          "(echo 'Authenticating with gcert:'; gcert)",
          shell=True)
      size = len(perf_files)
      items = zip(perf_files, [run_details_json] * size)
      urls: List[str] = []
      if self.browser_platform.is_remote:
        # Use loop, as we cannot easily serialize the remote platform.
        for perf_data_file, run_details in items:
          url = linux_perf_probe_pprof(perf_data_file, run_details,
                                       self.browser_platform)
          if url:
            urls.append(url)
      else:
        assert self.browser_platform == plt.PLATFORM
        with multiprocessing.Pool() as pool:
          urls = [
              url for url in pool.starmap(linux_perf_probe_pprof, items) if url
          ]
      try:
        if perf_files:
          # TODO: Add "combined" profile again
          pass
      except Exception as e:  # pylint: disable=broad-except
        logging.debug("Failed to run pprof: %s", e)
      return urls

  def _clean_up_temp_files(self, run: Run) -> None:
    if self.probe.cleanup_mode == CleanupMode.NEVER:
      logging.debug("%s: skipping cleanup", self.probe)
      return
    if self.probe.cleanup_mode == CleanupMode.AUTO:
      if not self.probe.run_pprof:
        logging.debug("%s: skipping auto cleanup without pprof upload",
                      self.probe)
        return
    for pattern in self.TEMP_FILE_PATTERNS:
      for file in run.out_dir.glob(pattern):
        file.unlink()


def prepare_linux_perf_env(platform: plt.Platform,
                           cwd: pth.RemotePath) -> Dict[str, str]:
  env: Dict[str, str] = dict(platform.environ)
  env["JITDUMPDIR"] = str(platform.absolute(cwd))
  return env


KB = 1024


def linux_perf_probe_inject_v8_symbols(
    perf_data_file: pth.RemotePath,
    platform: Optional[plt.Platform] = None) -> Optional[pth.RemotePath]:
  platform = platform or plt.PLATFORM
  assert platform.is_file(perf_data_file)
  output_file = perf_data_file.with_suffix(".data.jitted")
  assert not platform.exists(output_file)
  env = prepare_linux_perf_env(platform, perf_data_file.parent)
  try:
    # TODO: use remote chdir
    platform.sh(
        "perf",
        "inject",
        "--jit",
        f"--input={perf_data_file}",
        f"--output={output_file}",
        env=env)
  except plt.SubprocessError as e:
    if platform.file_size(perf_data_file) > 200 * KB:
      logging.warning("Failed processing: %s\n%s", perf_data_file, e)
    else:
      # TODO: investigate why almost all small perf.data files fail
      logging.debug("Failed processing small profile (likely empty): %s\n%s",
                    perf_data_file, e)
  if not platform.exists(output_file):
    return None
  return output_file


def linux_perf_probe_pprof(
    perf_data_file: pth.RemotePath,
    run_details: str,
    platform: Optional[plt.Platform] = None) -> Optional[str]:
  size = helper.get_file_size(perf_data_file)
  platform = platform or plt.PLATFORM
  env = prepare_linux_perf_env(platform, perf_data_file.parent)
  url = ""
  try:
    url = platform.sh_stdout(
        "pprof",
        "-flame",
        f"-add_comment={run_details}",
        perf_data_file,
        env=env,
    ).strip()
  except plt.SubprocessError as e:
    # Occasionally small .jitted files fail, likely due perf inject silently
    # failing?
    raw_perf_data_file = perf_data_file.with_suffix("")
    if (perf_data_file.suffix == ".jitted" and
        platform.exists(raw_perf_data_file)):
      logging.debug(
          "pprof best-effort: falling back to standard perf data "
          "without js symbols: %s \n"
          "Got failures for %s: %s", raw_perf_data_file, perf_data_file.name, e)
      try:
        perf_data_file = raw_perf_data_file
        url = platform.sh_stdout(
            "pprof",
            "-flame",
            f"-add_comment={run_details}",
            raw_perf_data_file,
        ).strip()
      except plt.SubprocessError as e2:
        logging.debug("pprof -flame failed: %s", e2)
    if not url:
      logging.warning("Failed processing: %s\n%s", perf_data_file, e)
      return None
  if perf_data_file.suffix == ".jitted":
    logging.info("PPROF (with js-symbols):")
  else:
    logging.info("PPROF (no js-symbols):")
  logging.info("  linux-perf:   %s %s", perf_data_file.name, size)
  logging.info("  pprof result: %s", url)
  return url


class AndroidProfilingContext(ProfilingContext):

  def __init__(self, probe: ProfilingProbe, run: Run) -> None:
    super().__init__(probe, run)
    self._simpleperf_process: Optional[subprocess.Popen] = None
    self._story_ready = False

  @cached_property
  def _renderer_pid_tid(self) -> Tuple[int, int]:
    assert self._story_ready, (
        "Fetching renderer PID/TID before the story is loaded could lead to "
        "the wrong PID/TID being used. This should never happen TM!")
    renderer_pid: Optional[int] = None
    renderer_main_tid: Optional[int] = None
    with self.run.actions("Get Renderer PID/TID") as actions:
      renderer_pid = actions.js(
          "return chrome?.benchmarking?.getRendererPid?.();")
      renderer_main_tid = actions.js(
          "return chrome?.benchmarking?.getRendererMainTid?.();")
    if renderer_pid is None or renderer_main_tid is None:
      error_message = (
          "Unable to get Renderer PID/TID from browser. "
          "Is the browser binary a sufficiently new version? "
          "For RENDERER_MAIN_ONLY/RENDERER_PROCESS_ONLY profiling, at least "
          "https://chromium-review.googlesource.com/c/chromium/src/+/5374765 "
          "is required.")
      logging.error(error_message)
      raise ValueError(error_message)
    return renderer_pid, renderer_main_tid

  def _generate_command_line(self) -> ListCmdArgs:
    renderer_pid: Optional[int] = None
    renderer_main_tid: Optional[int] = None
    if self.probe.target in (TargetMode.RENDERER_MAIN_ONLY,
                             TargetMode.RENDERER_PROCESS_ONLY):
      renderer_pid, renderer_main_tid = self._renderer_pid_tid
    return generate_simpleperf_command_line(
        self.probe.target,
        str(self.run.browser.path),
        renderer_pid,
        renderer_main_tid,
        self.probe.call_graph_mode,
        self.probe.frequency,
        self.probe.count,
        self.probe.cpu,
        self.probe.events,
        self.probe.grouped_events,
        self.probe.add_counters,
        self.result_path,
    )

  def _start_simpleperf(self) -> None:
    command_line = self._generate_command_line()
    logging.info("Starting simpleperf with command line: %s.", command_line)
    self._simpleperf_process = self.browser_platform.popen(
        *command_line, stdout=subprocess.PIPE, stderr=subprocess.STDOUT)
    # Wait a bit for simpleperf to start and (potentially) terminate on error.
    time.sleep(1)
    if self._simpleperf_process.poll():
      error_msg: str = ""
      if stdout := self._simpleperf_process.stdout:
        if isinstance(stdout, io.BufferedReader):
          error_msg = stdout.read().decode("utf-8")
          logging.error(error_msg)
      raise ValueError(f"Unable to start simpleperf. {error_msg}")
    atexit.register(self.stop_process)
    self.browser.performance_mark(self.runner,
                                  "crossbench-probe-profiling-start")

  def _get_simpleperf_pids(self) -> List[int]:
    simpleperf_pids = []
    for process in self.browser_platform.processes():
      if process["name"] == "simpleperf":
        simpleperf_pids.append(process["pid"])
    return simpleperf_pids

  def _stop_existing_simpleperf(self) -> None:
    for simpleperf_pid in self._get_simpleperf_pids():
      logging.warning("Terminating existing simpleperf process: %d.",
                      simpleperf_pid)
      self.browser_platform.terminate(simpleperf_pid)

  def _cpu_mask(self, cpus: Iterable) -> str:
    assert max(cpus) < 32, "Cpu index too high"
    mask = 0
    for cpu in cpus:
      mask |= (1 << cpu)
    return f"{mask:x}"

  def _pin_renderer_main_core(self, cpu: int):
    _, renderer_main_tid = self._renderer_pid_tid
    self.browser_platform.sh("taskset", "-p", self._cpu_mask([cpu]),
                             str(renderer_main_tid))

  def get_default_result_path(self) -> pth.RemotePath:
    return super().get_default_result_path().parent / "simpleperf.perf.data"

  def setup(self) -> None:
    assert self.browser.platform.is_android, (
        f"Expected Android platform, found {type(self.browser.platform)}.")
    assert self.browser.attributes.is_chromium_based, (
        f"Expected Chromium-based browser, found {type(self.browser)}.")
    if (self.browser.platform.is_android and
        self.browser.attributes.is_chromium_based):
      chromium = cast(Chromium, self.browser)
      # Set `--enable-benchmarking` explicitly for
      # retrieving Renderer PID, if needed.
      chromium.flags.set("--enable-benchmarking")
    self._stop_existing_simpleperf()

  def start(self) -> None:
    if not self.probe.start_profiling_after_setup:
      self._start_simpleperf()

  def start_story_run(self) -> None:
    self._story_ready = True
    if self.probe.pin_renderer_main_core is not None:
      self._pin_renderer_main_core(self.probe.pin_renderer_main_core)

    if self.probe.start_profiling_after_setup:
      self._start_simpleperf()

  def stop(self) -> None:
    self.stop_process()

  def stop_process(self) -> None:
    if self._simpleperf_process:
      helper.wait_and_kill(
          self._simpleperf_process, timeout=30, signal=signal.SIGINT)
      self._simpleperf_process = None
      self.browser.performance_mark(self.runner,
                                    "crossbench-probe-profiling-stop")

  def teardown(self) -> ProbeResult:
    return self.browser_result(trace=[self.result_path])


def generate_simpleperf_command_line(
    target: TargetMode,
    app_name: str,
    renderer_pid: Optional[int],
    renderer_main_tid: Optional[int],
    call_graph_mode: CallGraphMode,
    frequency: Optional[int],
    count: Optional[int],
    cpus: Tuple[int, ...],
    events: Tuple[str, ...],
    grouped_events: Tuple[str, ...],
    add_counters: Tuple[str, ...],
    output_path: pth.RemotePath,
) -> ListCmdArgs:
  command_line: ListCmdArgs = ["simpleperf", "record"]
  if target == TargetMode.RENDERER_MAIN_ONLY:
    assert renderer_main_tid is not None
    command_line.extend(["-t", str(renderer_main_tid)])
  elif target == TargetMode.RENDERER_PROCESS_ONLY:
    assert renderer_pid is not None
    command_line.extend(["-p", str(renderer_pid)])
  elif target == TargetMode.BROWSER_APP_ONLY:
    command_line.extend(["--app", app_name])
  else:  # TargetMode.SYSTEM_WIDE
    command_line.append("-a")
  if call_graph_mode == CallGraphMode.FRAME_POINTER:
    command_line.extend(["--call-graph", "fp"])
  elif call_graph_mode == CallGraphMode.DWARF:
    # Use "--post-unwind=yes" while unwinding with DWARF, to reduce
    # unwinding overhead during profiling.
    command_line.extend(["--call-graph", "dwarf", "--post-unwind=yes"])
  else:
    assert call_graph_mode == CallGraphMode.NO_CALL_GRAPH, (
        f"Invalid call_graph_mode: {call_graph_mode}")
  if frequency is not None:
    command_line.extend(["-f", str(frequency)])
  if count is not None:
    command_line.extend(["-c", str(count)])
  if cpus:
    command_line.extend(["--cpu", ",".join(map(str, cpus))])
  # Events and counters need to be provided after `-f` and `-c`.
  if events:
    command_line.extend(["-e", ",".join(events)])
  if grouped_events:
    command_line.extend(["--group", ",".join(grouped_events)])
  if add_counters:
    command_line.extend(["--add-counter", ",".join(add_counters)])
    # `--no-inherit` is required by simpleperf when `--add-counter` is used.
    command_line.append("--no-inherit")
  command_line.extend(["-o", output_path])
  return command_line
