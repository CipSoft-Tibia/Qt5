# Copyright 2023 The Chromium Authors
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

from __future__ import annotations

import logging
import pathlib
import subprocess
from typing import TYPE_CHECKING, Tuple, cast, Optional

from crossbench import helper
from crossbench.plt.android_adb import AndroidAdbPlatform
from crossbench.probes.probe import (Probe, ProbeConfigParser, ProbeContext,
                                     ProbeIncompatibleBrowser, ResultLocation)
from crossbench.probes.results import (ProbeResult, LocalProbeResult)

if TYPE_CHECKING:
  from crossbench.browsers.browser import Browser
  from crossbench.env import HostEnvironment
  from crossbench.runner.run import Run

_PERFETTO_CONFIG_REMOTE_DIR = pathlib.Path("/data/misc/perfetto-configs")
_PERFETTO_TRACE_REMOTE_DIR = pathlib.Path("/data/misc/perfetto-traces")


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
     config can be found at config/perfetto-probe.config.example.hjson.
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
    parser.add_argument("textproto", type=str, help="")
    parser.add_argument("perfetto_bin", type=str, default="perfetto", help="")
    return parser

  def __init__(self, textproto: str, perfetto_bin: str):
    super().__init__()
    assert textproto, "Please specify a tracing config"
    self._textproto = textproto
    self._perfetto_bin = perfetto_bin

  @property
  def key(self) -> Tuple[Tuple, ...]:
    return super().key + (
        ("textproto", self.textproto),
        ("perfetto_bin", str(self.perfetto_bin)),
    )

  @property
  def textproto(self) -> str:
    return self._textproto

  @property
  def perfetto_bin(self) -> str:
    return self._perfetto_bin

  def validate_browser(self, env: HostEnvironment, browser: Browser) -> None:
    super().validate_browser(env, browser)
    if not browser.platform.is_android:
      raise ProbeIncompatibleBrowser(self, browser, "Only supported on android")

  def log_run_result(self, run: Run) -> None:
    logging.critical("Perfetto trace is written into %s",
                     run.results[self].file)

  def get_context(self, run: Run) -> PerfettoProbeContext:
    return PerfettoProbeContext(self, run)


class PerfettoProbeContext(ProbeContext[PerfettoProbe]):

  def __init__(self, probe: PerfettoProbe, run: Run) -> None:
    super().__init__(probe, run)
    self._host_config_file: pathlib.Path = run.out_dir / "perfetto_config.textproto"
    self._browser_config_file: pathlib.Path = (
        _PERFETTO_CONFIG_REMOTE_DIR / "perfetto_config.textproto")
    self._browser_trace_file: pathlib.Path = (
        _PERFETTO_TRACE_REMOTE_DIR / "perfetto.trace.pb")
    self._pid: Optional[int] = None

  def setup(self) -> None:
    assert self._pid is None
    self.runner_platform.set_filecontents(self._host_config_file,
                                          self.probe.textproto)
    for p in self.browser_platform.processes():
      if p["name"] == "perfetto":
        logging.warning("PERFETTO: killing existing session pid: %s", p['pid'])
        self.browser_platform.terminate(p["pid"])

    assert isinstance(self.browser_platform, AndroidAdbPlatform)
    cast(AndroidAdbPlatform,
         self.browser_platform).push(self._host_config_file,
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
        self._browser_trace_file,
        capture_output=True)
    if proc.returncode > 0:
      logging.error("perfetto command failed with stderr: %s", proc.stderr)
      raise subprocess.CalledProcessError(proc.returncode, proc.args,
                                          proc.stdout, proc.stderr)

    self._pid = int(proc.stdout.decode("utf-8").rstrip())
    self.browser.performance_mark(self.runner,
                                  'crossbench-probe-perfetto-start')

  def stop(self) -> None:
    self.browser.performance_mark(self.runner, 'crossbench-probe-perfetto-stop')
    logging.info("PERFETTO: stopping")
    if not self._pid:
      raise RuntimeError("Perfetto was not started")
    # TODO(cbruni): replace with wait_and_terminate
    self.browser_platform.terminate(self._pid)
    try:
      for _ in helper.wait_with_backoff(helper.WaitRange(1, 30)):
        if not self.browser_platform.process_info(self._pid):
          break
    except TimeoutError:
      logging.error("perfetto process did not stop after 30s. "
                    "The trace might be incomplete.")
    self._pid = None

  def tear_down(self) -> ProbeResult:
    browser_trace_file = self.run.out_dir / self._browser_trace_file.name
    
    assert isinstance(self.browser_platform, AndroidAdbPlatform)
    cast(AndroidAdbPlatform,
         self.browser_platform).rsync(self._browser_trace_file,
                                      browser_trace_file)

    self.runner_platform.sh("gzip", browser_trace_file)
    browser_trace_file = (
        browser_trace_file.parent / f"{browser_trace_file.name}.gz")

    return LocalProbeResult(file=[browser_trace_file])
