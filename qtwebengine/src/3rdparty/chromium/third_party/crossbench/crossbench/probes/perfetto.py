# Copyright 2023 The Chromium Authors
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

from __future__ import annotations

import abc
import logging
import subprocess
from typing import TYPE_CHECKING, Iterable, Optional, cast

from crossbench import helper
from crossbench import path as pth
from crossbench.cli_helper import parse_remote_path
from crossbench.plt.android_adb import AndroidAdbPlatform
from crossbench.probes.probe import (Probe, ProbeConfigParser, ProbeContext,
                                     ProbeIncompatibleBrowser, ProbeKeyT,
                                     ResultLocation)
from crossbench.probes.results import LocalProbeResult, ProbeResult

if TYPE_CHECKING:
  from crossbench.browsers.browser import Browser
  from crossbench.env import HostEnvironment
  from crossbench.runner.groups.browsers import BrowsersRunGroup
  from crossbench.runner.run import Run

_PERFETTO_CONFIG_REMOTE_DIR = pth.RemotePath("/data/misc/perfetto-configs/")
_PERFETTO_TRACE_REMOTE_DIR = pth.RemotePath("/data/misc/perfetto-traces/")


class PerfettoProbe(Probe):
  """
  Android-only probe to collect Perfetto system traces that can be viewed on
  https://ui.perfetto.dev/.

  Recommended way to use:
  1. Go to https://ui.perfetto.dev/, click "Record new trace" and set up your
     preferred tracing options.
  2. Click "Recording command" and copy the textproto config part of the
     command.
  3. Paste it into the textproto field of the probe config. An example probe
     config can be found at config/doc/probe/perfetto.config.hjson.
  4. Specify the config via the --probe-config command-line flag.

  After the run, the trace will be found among the results as
  "perfetto.trace.pb.gz".
  """
  NAME = "perfetto"
  RESULT_LOCATION = ResultLocation.BROWSER

  IS_GENERAL_PURPOSE = True

  @classmethod
  def config_parser(cls) -> ProbeConfigParser:
    parser = super().config_parser()
    parser.add_argument(
        "textproto",
        type=str,
        help=("Serialized perfetto configuration. "
              "See probe instructions for more details"))
    parser.add_argument(
        "perfetto_bin",
        type=parse_remote_path,
        default="perfetto",
        help="Perfetto binary on the browser device")
    return parser

  def __init__(self, textproto: str, perfetto_bin: pth.RemotePath):
    super().__init__()
    if not textproto:
      raise ValueError("Please specify a tracing config")
    self._textproto = textproto
    if not perfetto_bin:
      raise ValueError("Please specify a perfetto binary.")
    self._perfetto_bin = perfetto_bin

  @property
  def key(self) -> ProbeKeyT:
    return super().key + (
        ("textproto", self.textproto),
        ("perfetto_bin", str(self.perfetto_bin)),
    )

  @property
  def textproto(self) -> str:
    return self._textproto

  @property
  def perfetto_bin(self) -> pth.RemotePath:
    return self._perfetto_bin

  @property
  def result_path_name(self) -> str:
    return "perfetto.trace.pb"

  def validate_browser(self, env: HostEnvironment, browser: Browser) -> None:
    super().validate_browser(env, browser)
    if not browser.platform.is_android:
      raise ProbeIncompatibleBrowser(self, browser, "Only supported on android")

  def attach(self, browser: Browser) -> None:
    assert browser.attributes.is_chromium_based
    browser.features.enable("EnablePerfettoSystemTracing")
    super().attach(browser)

  def log_run_result(self, run: Run) -> None:
    self._log_results([run])

  def log_browsers_result(self, group: BrowsersRunGroup) -> None:
    self._log_results(group.runs)

  def _log_results(self, runs: Iterable[Run]) -> None:
    logging.info("-" * 80)
    logging.critical("Perfetto trace results:")
    for run in runs:
      result_file = run.results[self].file
      logging.critical("  - %s : %s", result_file,
                       helper.get_file_size(result_file))

  def get_context(self, run: Run) -> PerfettoProbeContext:
    # TODO: support more platforms
    return AndroidPerfettoProbeContext(self, run)


class PerfettoProbeContext(ProbeContext[PerfettoProbe], metaclass=abc.ABCMeta):
  pass


class AndroidPerfettoProbeContext(PerfettoProbeContext):

  def __init__(self, probe: PerfettoProbe, run: Run) -> None:
    super().__init__(probe, run)
    self._host_config_file: pth.LocalPath = (
        run.out_dir / "perfetto_config.textproto")
    self._browser_config_file: pth.RemotePath = (
        _PERFETTO_CONFIG_REMOTE_DIR / "perfetto_config.textproto")
    self._pid: Optional[int] = None

  def get_default_result_path(self) -> pth.RemotePath:
    return _PERFETTO_TRACE_REMOTE_DIR / "perfetto.trace.pb"

  @property
  def browser_platform(self) -> AndroidAdbPlatform:
    browser_platform = super().browser_platform
    assert isinstance(browser_platform, AndroidAdbPlatform)
    return cast(AndroidAdbPlatform, browser_platform)

  def setup(self) -> None:
    assert self._pid is None
    for p in self.browser_platform.processes():
      if p["name"] == "perfetto":
        logging.warning("PERFETTO: killing existing session pid: %s", p["pid"])
        self.browser_platform.terminate(p["pid"])

    if not self.browser_platform.which(self.probe.perfetto_bin):
      raise ValueError(
          f"perfetto bin '{self.probe.perfetto_bin}' cannot be found "
          f"on {self.browser_platform}")

    self.runner_platform.set_file_contents(self._host_config_file,
                                           self.probe.textproto)
    self.browser_platform.push(self._host_config_file,
                               self._browser_config_file)

  def start(self) -> None:
    logging.info("PERFETTO: starting")
    proc = self.browser_platform.sh(
        self.probe.perfetto_bin,
        "--background",
        "--config",
        self._browser_config_file,
        "--txt",
        "--out",
        self.result_path,
        capture_output=True)
    if proc.returncode > 0:
      logging.error("perfetto command failed with stderr: %s", proc.stderr)
      raise subprocess.CalledProcessError(proc.returncode, proc.args,
                                          proc.stdout, proc.stderr)

    self._pid = int(proc.stdout.decode("utf-8").rstrip())
    self.browser.performance_mark(self.runner,
                                  "crossbench-probe-perfetto-start")

  def stop(self) -> None:
    self.browser.performance_mark(self.runner, "crossbench-probe-perfetto-stop")
    logging.info("PERFETTO: stopping")
    if not self._pid:
      raise RuntimeError("Perfetto was not started")
    # TODO(cbruni): replace with wait_and_terminate
    self.browser_platform.terminate(self._pid)
    try:
      for _ in helper.WaitRange(1, 30).wait_with_backoff():
        if not self.browser_platform.process_info(self._pid):
          break
    except TimeoutError:
      logging.error("perfetto process did not stop after 30s. "
                    "The trace might be incomplete.")
    self._pid = None

  def teardown(self) -> ProbeResult:
    # Copy files:
    browser_result = self.browser_result(file=[self.result_path])
    local_result_file = browser_result.file
    assert local_result_file.is_file(), (
        f"Could not copy perfetto results: {local_result_file}")

    self.runner_platform.sh("gzip", local_result_file)
    local_result_file = local_result_file.with_suffix(
        f"{local_result_file.suffix}.gz")

    return LocalProbeResult(trace=(local_result_file,))
