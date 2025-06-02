# Copyright 2023 The Chromium Authors
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

from __future__ import annotations

import atexit
import pathlib
import subprocess
from typing import TYPE_CHECKING, Optional, TextIO, Tuple

from crossbench import cli_helper
from crossbench.probes.probe import (Probe, ProbeConfigParser, ProbeContext,
                                     ProbeValidationError, ResultLocation)
from crossbench.probes.results import ProbeResult

if TYPE_CHECKING:
  from crossbench.browsers.browser import Browser
  from crossbench.env import HostEnvironment
  from crossbench.runner.run import Run


class DTraceProbe(Probe):
  """
  Probe to collect data using DTrace.
  """

  NAME = "dtrace"
  RESULT_LOCATION = ResultLocation.BROWSER

  @classmethod
  def config_parser(cls) -> ProbeConfigParser:
    parser = super().config_parser()
    parser.add_argument(
        "script_path", type=cli_helper.parse_non_empty_file_path)
    return parser

  def __init__(self, script_path: pathlib.Path):
    super().__init__()
    self._script_path = script_path.resolve()

  @property
  def key(self) -> Tuple[Tuple, ...]:
    return super().key + (("script_path", str(self.script_path)),)

  @property
  def script_path(self) -> pathlib.Path:
    return self._script_path

  def validate_browser(self, env: HostEnvironment, browser: Browser) -> None:
    super().validate_browser(env, browser)
    self.expect_macos(browser)

    # Check that it is possible to execute 'sudo dtrace' without prompting for a
    # password. The best way to do this is to actually run the command and check
    # the return value of `sudo`.
    #
    # Under normal usage, DTrace expects an input script file and returns '2'
    # when it is missing. To force a return value of zero, without actually
    # providing a valid script file, the '-l -P $dtrace_probe_name' argument is
    # used, which tells DTrace to simply print all DTrace probes (do not confuse
    # with crossbench probes) whose name matches $dtrace_probe_name. This will
    # ensure the command either succeeds or fails fast. Use a non-existant probe
    # name to reduce output size
    dtrace_probe_name = 'nonexistantprobename'
    # Execute and check the returncode, while ignoring output.
    try:
      browser.platform.sh(
          "sudo",
          "-n",
          "dtrace",
          "-l",
          "-P",
          dtrace_probe_name,
          stdout=subprocess.DEVNULL,
          stderr=subprocess.STDOUT)
    except subprocess.CalledProcessError as e:
      raise ProbeValidationError(
          self, "Cannot execute 'sudo dtrace'. "
          "This probe will fail to start.") from e

  def get_context(self, run: Run) -> DTraceProbeContext:
    return DTraceProbeContext(self, run)


class DTraceProbeContext(ProbeContext[DTraceProbe]):

  def __init__(self, probe: DTraceProbe, run: Run) -> None:
    super().__init__(probe, run)
    self._script_path: pathlib.Path = probe.script_path
    self._output_path: pathlib.Path = self.result_path.with_suffix(
        ".output.txt")
    self._log_path: pathlib.Path = self.result_path.with_suffix(".log")
    self._dtrace_process: Optional[subprocess.Popen] = None
    self._log_file: Optional[TextIO] = None
    atexit.register(self.stop_dtrace_process)

  def start(self) -> None:
    self._log_file = open(self._log_path, "w")
    self._dtrace_process = self.browser_platform.popen(
        'sudo',
        '-n',
        'dtrace',
        "-p",
        str(self.browser_pid),
        "-o",
        self._output_path,
        "-s",
        self._script_path,
        stdout=self._log_file,
        stderr=self._log_file,
    )
    assert self._dtrace_process is not None, ("Could not start DTrace")

  def stop(self) -> None:
    if not self._dtrace_process:
      return
    # DTrace will close by itself once the browser exits.
    returncode = self._dtrace_process.poll()
    if returncode is not None:
      # Print an error message if DTrace was already closed.
      if returncode == 0:
        raise RuntimeError("DTrace exited early without errors.")
      raise RuntimeError(f"DTrace exited early with error {returncode}.\n"
                         f"Check {self._log_path} for the program's log.")

  def tear_down(self) -> ProbeResult:
    self.stop_dtrace_process()
    assert self._log_file, "Did not open log file."
    self._log_file.close()
    return self.browser_result(file=(self._output_path,))

  def stop_dtrace_process(self) -> None:
    if self._dtrace_process:
      try:
        # Wait for the process to terminate normally.
        returncode = self._dtrace_process.wait(timeout=5)
        if returncode != 0:
          raise RuntimeError(f"DTrace exited with error {returncode}.\n"
                             f"Check {self._log_path} for the program's log.")
      except subprocess.TimeoutExpired:
        # DTrace took too long to terminate. Send SIGKILL.
        # Note: Not using .kill() because the process was started with sudo so
        # it would raise an PermissionError exception.
        subprocess.run(
            ["sudo", "-n", "kill", "-SIGKILL", f"{self._dtrace_process.pid}"],
            check=True)
