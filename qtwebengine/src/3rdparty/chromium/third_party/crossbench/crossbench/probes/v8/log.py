# Copyright 2022 The Chromium Authors
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

from __future__ import annotations

import logging
import multiprocessing
import os
import re
import subprocess
from typing import TYPE_CHECKING, Iterable, List, Optional, cast

from crossbench import cli_helper, compat, helper, plt
from crossbench.browsers.browser import Browser
from crossbench.browsers.chromium.chromium import Chromium
from crossbench.flags.js_flags import JSFlags
from crossbench.helper.path_finder import V8ToolsFinder
from crossbench.probes.chromium_probe import ChromiumProbe
from crossbench.probes.probe import (ProbeConfigParser, ProbeContext, ProbeKeyT,
                                     ResultLocation)
from crossbench.probes.results import ProbeResult

if TYPE_CHECKING:
  from crossbench.env import HostEnvironment
  from crossbench.path import LocalPath, RemotePath
  from crossbench.runner.groups import BrowsersRunGroup
  from crossbench.runner.run import Run

_PROF_FLAG = "--prof"
_LOG_ALL_FLAG = "--log-all"


class V8LogProbe(ChromiumProbe):
  """
  Chromium-only probe that produces a v8.log file with detailed internal V8
  performance and logging information.
  This file can be used by tools hosted on http://v8.dev/tools.
  If prof == true, this probe will try to generate profview.json files for
  http://v8.dev/tools/head/profview. See de d8_binary and v8_checkout
  config-properties for more details.
  """
  NAME = "v8.log"
  RESULT_LOCATION = ResultLocation.BROWSER

  _FLAG_RE = re.compile("^--(prof|log-|no-log-).*$")

  @classmethod
  def config_parser(cls) -> ProbeConfigParser:
    parser = super().config_parser()
    parser.add_argument(
        "log_all",
        type=bool,
        default=True,
        help="Enable all v8 logging (equivalent to --log-all)")
    parser.add_argument(
        "prof",
        type=bool,
        default=True,
        help="Enable v8-profiling (equivalent to --prof)")
    parser.add_argument(
        "profview",
        type=bool,
        default=True,
        help=("Enable v8-profiling and generate profview.json files for "
              "http://v8.dev/tools/head/profview"))
    parser.add_argument(
        "js_flags",
        type=str,
        default=[],
        is_list=True,
        help="Manually pass --log-.* flags to V8")
    parser.add_argument(
        "d8_binary",
        type=cli_helper.parse_file_path,
        help="Path to a D8 binary for extended log processing."
        "If not specified the $D8_PATH env variable is used and/or "
        "default build locations are tried.")
    parser.add_argument(
        "v8_checkout",
        type=cli_helper.parse_dir_path,
        help="Path to a V8 checkout for extended log processing."
        "If not specified it is auto inferred from either the provided"
        "d8_binary or standard installation locations.")
    return parser

  def __init__(
      self,
      log_all: bool = True,
      prof: bool = True,
      profview: bool = True,
      js_flags: Optional[Iterable[str]] = None,
      # TODO: support remote platform
      d8_binary: Optional[LocalPath] = None,
      v8_checkout: Optional[LocalPath] = None) -> None:
    super().__init__()
    self._profview: bool = profview
    self._js_flags = JSFlags()
    self._d8_binary: Optional[LocalPath] = d8_binary
    self._v8_checkout: Optional[LocalPath] = v8_checkout
    assert isinstance(log_all,
                      bool), (f"Expected bool value, got log_all={log_all}")
    assert isinstance(prof, bool), f"Expected bool value, got log_all={prof}"
    if log_all:
      self._js_flags.set(_LOG_ALL_FLAG)
    elif prof:
      self._js_flags.set(_PROF_FLAG)
    elif profview:
      raise ValueError(f"{self}: Need prof:true with profview:true")
    js_flags = js_flags or []
    for flag in js_flags:
      if self._FLAG_RE.match(flag):
        self._js_flags.set(flag)
      else:
        raise ValueError(f"{self}: Non-v8.log-related flag detected: {flag}")
    if len(self._js_flags) == 0:
      raise ValueError(f"{self}: V8LogProbe has no effect")

  @property
  def key(self) -> ProbeKeyT:
    return super().key + (
        ("profview", self._profview),
        ("js_flags", str(self.js_flags)),
        ("d8_binary", str(self._d8_binary)),
        ("v8_checkout", str(self._v8_checkout)),
    )

  @property
  def js_flags(self) -> JSFlags:
    return self._js_flags.copy()

  def validate_env(self, env: HostEnvironment) -> None:
    super().validate_env(env)
    if env.runner.repetitions != 1:
      env.handle_warning(f"Probe({self.NAME}) cannot merge data over multiple "
                         f"repetitions={env.runner.repetitions}.")

  def validate_browser(self, env: HostEnvironment, browser: Browser) -> None:
    super().validate_browser(env, browser)
    # --prof sometimes causes issues on enterprise chrome on linux.
    if _PROF_FLAG not in self._js_flags:
      return
    if not browser.platform.is_linux or browser.major_version <= 106:
      return
    for search_path in cast(plt.LinuxPlatform, browser.platform).SEARCH_PATHS:
      if compat.is_relative_to(browser.path, search_path):
        logging.error(
            "Probe with V8 --prof might not work with enterprise profiles")

  def attach(self, browser: Browser) -> None:
    super().attach(browser)
    assert isinstance(browser, Chromium)

    browser = cast(Chromium, browser)
    browser.flags.set("--no-sandbox")
    browser.js_flags.update(self._js_flags)

  def process_log_files(self, log_files: List[RemotePath]) -> List[RemotePath]:
    if not self._profview:
      return []
    platform = self.runner_platform
    finder = V8ToolsFinder(platform, self._d8_binary, self._v8_checkout)
    if not finder.d8_binary or not finder.tick_processor or not log_files:
      logging.warning("Did not find $D8_PATH for profview processing.")
      return []
    logging.info(
        "PROBE v8.log: generating profview json data "
        "for %d v8.log files. (slow)", len(log_files))
    logging.debug("v8.log files: %s", log_files)
    if platform.is_remote:
      # TODO: fix, currently unused
      # Use loop, as we cannot easily serialize the remote platform.
      return [
          _process_profview_json(finder.d8_binary, finder.tick_processor,
                                 log_file) for log_file in log_files
      ]
    assert platform == plt.PLATFORM
    with multiprocessing.Pool(processes=4) as pool:
      return list(
          pool.starmap(_process_profview_json,
                       [(finder.d8_binary, finder.tick_processor, log_file)
                        for log_file in log_files]))

  def get_context(self, run: Run) -> V8LogProbeContext:
    return V8LogProbeContext(self, run)

  def log_browsers_result(self, group: BrowsersRunGroup) -> None:
    runs: List[Run] = list(run for run in group.runs if self in run.results)
    if not runs:
      return
    logging.info("-" * 80)
    logging.critical("v8.log results:")
    logging.info("  *.v8.log:        https://v8.dev/tools/head/system-analyzer")
    logging.info("  *.profview.json: https://v8.dev/tools/head/profview")
    logging.info("- " * 40)
    # Iterate over all runs again, to get proper indices:
    for i, run in enumerate(group.runs):
      if self not in run.results:
        continue
      log_files = run.results[self].file_list
      if not log_files:
        continue
      logging.info("Run %d: %s", i + 1, run.name)
      largest_log_file = log_files[0]
      logging.critical("    %s : %s", largest_log_file,
                       helper.get_file_size(largest_log_file))
      if len(log_files) > 1:
        logging.info("    %s/.*v8.log: %d files", largest_log_file.parent,
                     len(log_files))
      profview_files = run.results[self].json_list
      if not profview_files:
        continue
      largest_profview_file = profview_files[0]
      logging.critical("    %s : %s", largest_profview_file,
                       helper.get_file_size(largest_profview_file))
      if len(profview_files) > 1:
        logging.info("    %s/*.profview.json: %d more files",
                     largest_profview_file.parent, len(profview_files))


class V8LogProbeContext(ProbeContext[V8LogProbe]):

  def get_default_result_path(self) -> RemotePath:
    log_dir = super().get_default_result_path()
    self.browser_platform.mkdir(log_dir)
    return log_dir / self.probe.result_path_name

  def setup(self) -> None:
    self.session.extra_js_flags["--logfile"] = str(self.result_path)

  def start(self) -> None:
    pass

  def stop(self) -> None:
    pass

  def teardown(self) -> ProbeResult:
    log_dir = self.result_path.parent
    log_files = helper.sort_by_file_size(
        self.browser_platform.glob(log_dir, "*-v8.log"), self.browser_platform)
    # Only convert a v8.log file with profile ticks.
    json_list: List[RemotePath] = []
    maybe_js_flags = getattr(self.browser, "js_flags", {})
    if _PROF_FLAG in maybe_js_flags or _LOG_ALL_FLAG in maybe_js_flags:
      with helper.Spinner():
        json_list = self.probe.process_log_files(log_files)
    return self.browser_result(file=tuple(log_files), json=json_list)


def _process_profview_json(d8_binary: RemotePath, tick_processor: RemotePath,
                           log_file: RemotePath) -> RemotePath:
  env = os.environ.copy()
  # TODO: support remote platforms
  platform = plt.PLATFORM
  # The tick-processor scripts expect D8_PATH to point to the parent dir.
  env["D8_PATH"] = str(platform.local_path(d8_binary).parent.resolve())
  result_json = log_file.with_suffix(".profview.json")
  with platform.local_path(result_json).open("w", encoding="utf-8") as f:
    platform.sh(
        tick_processor,
        "--preprocess",
        log_file,
        env=env,
        stdout=f,
        stderr=subprocess.PIPE)
  return result_json
