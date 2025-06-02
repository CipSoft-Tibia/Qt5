# Copyright 2023 The Chromium Authors
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

from __future__ import annotations

import abc
import enum
import json
from typing import TYPE_CHECKING, List, Optional, Tuple, cast

from selenium.webdriver.safari.options import Options as SafariOptions

from crossbench import compat
from crossbench.browsers.chromium.webdriver import ChromiumWebDriver
from crossbench.browsers.firefox.firefox import Firefox
from crossbench.browsers.safari.webdriver import SafariWebDriver
from crossbench.probes.probe import (Probe, ProbeConfigParser, ProbeContext,
                                     ProbeIncompatibleBrowser, ProbeResult,
                                     ProbeValidationError, ResultLocation)

if TYPE_CHECKING:
  import pathlib

  from selenium.webdriver.common.options import BaseOptions

  from crossbench.browsers.browser import Browser
  from crossbench.env import HostEnvironment
  from crossbench.runner.run import Run


@enum.unique
class MozProfilerStartupFeatures(compat.StrEnumWithHelp):
  """Options for MOZ_PROFILER_STARTUP_FEATURES env var.
    Extracted via MOZ_PROFILER_HELP=1 ./firefox-nightly-en/firefox
    """
  JAVA = ("java", "Profile Java code, Android only")
  JS = ("js", "Get the JS engine to expose the JS stack to the profiler")
  LEAF = ("leaf", "Include the C++ leaf node if not stackwalking")
  MAINTHREADIO = ("mainthreadio", "Add main thread file I/O")
  FILEIO = ("fileio",
            "Add file I/O from all profiled threads, implies mainthreadio")
  FILEIOALL = ("fileioall", "Add file I/O from all threads, implies fileio")
  NOIOSTACKS = ("noiostacks",
                "File I/O markers do not capture stacks, to reduce overhead")
  SCREENSHOTS = ("screenshots",
                 "Take a snapshot of the window on every composition")
  SEQSTYLE = ("seqstyle", "Disable parallel traversal in styling")
  STACKWALK = ("stackwalk",
               "Walk the C++ stack, not available on all platforms")
  TASKTRACER = ("tasktracer", "Start profiling with feature TaskTracer")
  THREADS = ("threads", "Profile the registered secondary threads")
  JSTRACER = ("jstracer", "Enable tracing of the JavaScript engine")
  JSALLOCATIONS = ("jsallocations",
                   "Have the JavaScript engine track allocations")
  NOSTACKSAMPLING = (
      "nostacksampling",
      "Disable all stack sampling: Cancels 'js', 'leaf', 'stackwalk' and labels"
  )
  PREFERENCEREADS = ("preferencereads", "Track when preferences are read")
  NATIVEALLOCATIONS = (
      "nativeallocations",
      "Collect the stacks from a smaller subset of all native allocations, "
      "biasing towards collecting larger allocations")
  IPCMESSAGES = ("ipcmessages",
                 "Have the IPC layer track cross-process messages")
  AUDIOCALLBACKTRACING = ("audiocallbacktracing", "Audio callback tracing")
  CPU = ("cpu", "CPU utilization")


class FirefoxProfilerEnvVars(compat.StrEnum):
  # If set to any value other than '' or '0'/'N'/'n', starts the
  # profiler immediately on start-up.
  STARTUP = "MOZ_PROFILER_STARTUP"
  # Contains a comma-separated list of MozProfilerStartupFeatures.
  STARTUP_FEATURES = "MOZ_PROFILER_STARTUP_FEATURES"
  # If set, the profiler saves a profile to the named file on shutdown.
  SHUTDOWN = "MOZ_PROFILER_SHUTDOWN"


class BrowserProfilingProbe(Probe):
  """
  Browser profiling for generating in-browser performance profiles:
  - Firefox https://profiler.firefox.com/
  - Chrome: https://developer.chrome.com/docs/devtools/
  - Safari: Timelines https://developer.apple.com/safari/tools
  """
  NAME = "browser-profiling"
  RESULT_LOCATION = ResultLocation.BROWSER
  IS_GENERAL_PURPOSE = True

  @classmethod
  def config_parser(cls) -> ProbeConfigParser:
    parser = super().config_parser()
    parser.add_argument(
        "moz_profiler_startup_features",
        type=MozProfilerStartupFeatures,
        is_list=True,
        default=[])
    return parser

  def __init__(self,
               moz_profiler_startup_features: Optional[
                   List[MozProfilerStartupFeatures]] = None):
    super().__init__()
    self._moz_profiler_startup_features: List[
        MozProfilerStartupFeatures] = moz_profiler_startup_features or []

  @property
  def key(self) -> Tuple[Tuple, ...]:
    return super().key + (("moz_profiler_startup_features",
                           tuple(self.moz_profiler_startup_features)),)

  @property
  def moz_profiler_startup_features(self) -> List[MozProfilerStartupFeatures]:
    return self._moz_profiler_startup_features

  def validate_browser(self, env: HostEnvironment, browser: Browser) -> None:
    super().validate_browser(env, browser)
    if browser.platform.is_remote:
      raise ProbeValidationError(
          self, f"Only works on local browser, but got {browser}.")
    if isinstance(browser, (ChromiumWebDriver, SafariWebDriver)):
      return
    if isinstance(browser, Firefox):
      browser_env = browser.platform.environ
      for env_var in list(FirefoxProfilerEnvVars):
        if env_var.value in browser_env:
          env.handle_warning(
              f"Probe({self}) conflicts with existing "
              f"env[{env_var.value}]={browser_env[env_var.value]}")
    raise ProbeIncompatibleBrowser(self, browser)

  def get_context(self, run: Run) -> BrowserProfilingProbeContext:
    if isinstance(run.browser, ChromiumWebDriver):
      return ChromiumWebDriverBrowserProfilerProbeContext(self, run)
    if isinstance(run.browser, Firefox):
      return FirefoxBrowserProfilerProbeContext(self, run)
    if isinstance(run.browser, SafariWebDriver):
      return SafariWebdriverBrowserProfilerProbeContext(self, run)
    raise NotImplementedError(
        f"Probe({self}): Unsupported browser: {run.browser}")


class BrowserProfilingProbeContext(
    ProbeContext[BrowserProfilingProbe], metaclass=abc.ABCMeta):

  def setup(self) -> None:
    pass

  def start(self) -> None:
    pass

  def stop(self) -> None:
    pass


class ChromiumWebDriverBrowserProfilerProbeContext(BrowserProfilingProbeContext
                                                  ):

  def get_default_result_path(self) -> pathlib.Path:
    return (super().get_default_result_path().parent /
            f"{self.browser.type}.profile.json")

  @property
  def chromium(self) -> ChromiumWebDriver:
    return cast(ChromiumWebDriver, self.browser)

  def start(self) -> None:
    self.chromium.start_profiling()

  def stop(self) -> None:
    with self.run.actions(f"Probe({self.probe}): extract DevTools profile."):
      profile = self.chromium.stop_profiling()
      with self.result_path.open("w", encoding="utf-8") as f:
        json.dump(profile, f)

  def tear_down(self) -> ProbeResult:
    return self.browser_result(json=[self.result_path])


class FirefoxBrowserProfilerProbeContext(BrowserProfilingProbeContext):

  def get_default_result_path(self) -> pathlib.Path:
    return super().get_default_result_path().parent / "firefox.profile.json"

  def setup(self) -> None:
    env = self.browser.platform.environ
    env[FirefoxProfilerEnvVars.STARTUP] = "y"
    if self.probe.moz_profiler_startup_features:
      env[FirefoxProfilerEnvVars.STARTUP_FEATURES] = ",".join(
          str(feature) for feature in self.probe.moz_profiler_startup_features)
    env[FirefoxProfilerEnvVars.SHUTDOWN] = str(self.result_path)

  def tear_down(self) -> ProbeResult:
    env = self.browser.platform.environ
    del env[FirefoxProfilerEnvVars.STARTUP]
    del env[FirefoxProfilerEnvVars.STARTUP_FEATURES]
    del env[FirefoxProfilerEnvVars.SHUTDOWN]
    return self.browser_result(json=[self.result_path])


class SafariWebdriverBrowserProfilerProbeContext(BrowserProfilingProbeContext):

  def get_default_result_path(self) -> pathlib.Path:
    return super().get_default_result_path().parent / "safari.timeline.json"

  def setup_selenium_options(self, options: BaseOptions) -> None:
    assert isinstance(options, SafariOptions)
    cast(SafariOptions, options).automatic_profiling = True

  def stop(self) -> None:
    # TODO: Update this mess when Safari supports a command-line option
    # to download the profile.
    # Manually safe the profile using apple script to navigate the safari UI
    # Stop profiling.
    self.browser_platform.exec_apple_script("""
      tell application "System Events"
        keystroke "T" using {command down, option down, shift down}
      end tell""")
    # TODO: explicitly focus the developer pane
    # Focus the Developer Tools split pane and use CMD-S to save the profile.
    self.browser_platform.exec_apple_script(f"""
      tell application "System Events"
        keystroke "S" using command down
        tell window "Save As"
          delay 0.5
          keystroke "g" using {{command down, shift down}}
          delay 0.5
          # Send DELETE key input to clear the current text input.
          key code 51
          keystroke "{self.result_path}"
          delay 0.5
          keystroke return
          delay 0.5
          keystroke return
        end tell
      end tell""")

  def tear_down(self) -> ProbeResult:
    return self.browser_result(json=[self.result_path])
