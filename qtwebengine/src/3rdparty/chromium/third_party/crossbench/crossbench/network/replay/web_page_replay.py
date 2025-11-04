# Copyright 2023 The Chromium Authors
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

from __future__ import annotations

import abc
import atexit
import logging
import re
import shlex
import subprocess
import time
from typing import TYPE_CHECKING, Iterable, Optional, TextIO, Tuple

from crossbench import cli_helper, helper
from crossbench.helper.path_finder import WprGoToolFinder
from crossbench.plt import PLATFORM, Platform, TupleCmdArgs

if TYPE_CHECKING:
  from crossbench.path import LocalPath

_WPR_PORT_RE = re.compile(r".*Starting server on "
                          r"(?P<protocol>http|https)://"
                          r"(?P<host>[^:]+):"
                          r"(?P<port>\d+)")


class WprStartupError(RuntimeError):
  pass


class WprBase(abc.ABC):
  NAME: str = ""

  _key_file: LocalPath
  _cert_file: LocalPath

  def __init__(self,
               archive_path: LocalPath,
               bin_path: LocalPath,
               http_port: int = 0,
               https_port: int = 0,
               host: str = "127.0.0.1",
               inject_scripts: Optional[Iterable[LocalPath]] = None,
               key_file: Optional[LocalPath] = None,
               cert_file: Optional[LocalPath] = None,
               log_path: Optional[LocalPath] = None,
               platform: Platform = PLATFORM):
    self._platform: Platform = platform
    self._process: Optional[subprocess.Popen] = None
    self._log_path: Optional[LocalPath] = None
    if log_path:
      self._log_path = cli_helper.parse_not_existing_path(log_path)
    self._log_file: Optional[TextIO] = None
    self._bin_path = cli_helper.parse_non_empty_file_path(bin_path)
    self._go_cmd: TupleCmdArgs = ()
    wpr_root: LocalPath
    if self._bin_path.suffix == ".go":
      # `go` binary is required to run a Go source file (`wpr.go`).
      if local_go := self._platform.which("go"):
        self._go_cmd = (local_go, "run", self._bin_path)
      else:
        raise ValueError(f"'go' binary not available on {self._platform}")
      wpr_root = self._bin_path.parents[1]
    else:
      # Assuming the binary path is precompiled and executable.
      self._go_cmd = (self._bin_path,)
      if local_wpr_go := WprGoToolFinder(self._platform).path:
        wpr_root = self._platform.local_path(local_wpr_go.parents[1])
      else:
        raise ValueError(
            f"Could not find web_page_replay_go on {self._platform}")
    self._archive_path = self._validate_archive_path(archive_path)
    if http_port == https_port:
      raise ValueError("http_port must be different from https_port, "
                       f"but got twice: {http_port}")
    self._http_port = http_port
    self._https_port = https_port
    self._num_parsed_ports: int = 0

    self._host: str = host

    if key_file:
      self._key_file = key_file
    else:
      self._key_file = wpr_root / "ecdsa_key.pem"
    if not self._key_file.is_file():
      raise ValueError(f"Could not find ecdsa_key.pem file: {self._key_file}")

    if cert_file:
      self._cert_file = cert_file
    else:
      self._cert_file = wpr_root / "ecdsa_cert.pem"
    if not self._cert_file.is_file():
      raise ValueError(f"Could not find ecdsa_cert.pem file: {self._cert_file}")

    if inject_scripts is None:
      inject_scripts = [wpr_root / "deterministic.js"]
    for script in inject_scripts:
      if "," in str(script):
        raise ValueError(f"Injected script path cannot contain ',': {script}")
      if not script.is_file():
        raise ValueError(f"Injected script does not exist: {script}")
    self._inject_scripts: Tuple[LocalPath, ...] = tuple(inject_scripts)

  @abc.abstractmethod
  def _validate_archive_path(self, path: LocalPath) -> LocalPath:
    pass

  @property
  def http_port(self) -> int:
    return self._http_port

  @property
  def https_port(self) -> int:
    return self._https_port

  @property
  def host(self) -> str:
    return self._host

  @property
  def cert_file(self) -> LocalPath:
    return self._cert_file

  @property
  @abc.abstractmethod
  def cmd(self) -> TupleCmdArgs:
    pass

  @property
  def base_cmd_flags(self) -> TupleCmdArgs:
    cmd: TupleCmdArgs = (
        f"--http_port={self._http_port}",
        f"--https_port={self._https_port}",
        f"--https_key_file={self._key_file}",
        f"--https_cert_file={self._cert_file}",
    )
    if self._inject_scripts is not None:
      injected_scripts = ",".join(str(path) for path in self._inject_scripts)
      cmd += (f"--inject_scripts={injected_scripts}",)
    return cmd

  def start(self):
    go_cmd: TupleCmdArgs = self._go_cmd + self.cmd
    logging.info("STARTING WPR %s", shlex.join(map(str, go_cmd)))
    self._num_parsed_ports = 0
    try:
      if self._log_path:
        self._log_file = self._log_path.open("w", encoding="utf-8")  # pylint: disable=consider-using-with
      with helper.ChangeCWD(self._bin_path.parent):
        logging.debug("Logging to %s", self._log_path)
        self._process = self._platform.popen(
            *go_cmd, stdout=self._log_file, stderr=self._log_file)

      if not self._process:
        raise WprStartupError(f"Could not start {type(self).__name__}")

      atexit.register(self.stop)
      logging.info("WPR: waiting for startup")
      self._wait_for_startup()
      logging.info("WPR: Starting wpr.go %s: DONE", self.NAME)
    except BaseException as e:
      if isinstance(e, Exception):
        logging.debug("WPR got startup errors: %s %s", type(e), e)
      force_shutdown = isinstance(e, WprStartupError)
      self.stop(force_shutdown)
      self._handle_startup_error()
      raise

  def _handle_startup_error(self):
    logging.error("WPR: Could not start %s", type(self).__name__)
    if not self._log_path or not self._log_path.exists():
      return
    logging.error("WPR: Check log files %s", self._log_path)
    try:
      with self._log_path.open("r", encoding="utf-8") as f:
        log_lines = list(f.readlines())
        logging.error("  %s", "  ".join(log_lines[-4:]))
    except Exception as e:  # pylint: disable=broad-except
      logging.debug("Got exception while reading wpr log file: %s", e)

  def _wait_for_startup(self) -> None:
    assert self._process, "process not started"
    assert self._log_path, "missing log_path"
    assert self._num_parsed_ports == 0, "WPR did not shut down correctly."
    time.sleep(1)
    with self._log_path.open("r", encoding="utf-8") as log_file:
      while self._process.poll() is None:
        line = log_file.readline()
        if not line:
          time.sleep(0.1)
          continue
        if self._parse_wpr_log_line(line):
          break
    if self._process.poll():
      self._raise_startup_failure()
    time.sleep(0.1)
    try:
      with self._open_wpr_cmd_url("generate-200") as r:
        if r.status == 200:
          return
    except Exception as e:  # pylint: disable=broad-except
      logging.debug("Could not query wpr server: %s", e)
    self._raise_startup_failure()

  def _raise_startup_failure(self) -> None:
    raise WprStartupError("Could not start wpr.go.\n"
                          f"See log for more details: {self._log_path}")

  def _parse_wpr_log_line(self, line: str) -> bool:
    if "Failed to start server on" in line:
      logging.error(line)
      raise WprStartupError(
          f"Could not start wpr.go server, address in use: {line}")
    line = line.strip()
    if match := _WPR_PORT_RE.match(line):
      protocol = match["protocol"].lower()
      port = int(match["port"])
      if protocol == "http":
        self._http_port = port
        self._num_parsed_ports += 1
      elif protocol == "https":
        self._https_port = port
        self._num_parsed_ports += 1
      else:
        logging.error("WPR: got invalid protocol: %s", line)
      self._host = match["host"]
      if not self._host:
        raise WprStartupError(f"WPR: could not parse host from: {line}")

    if self._num_parsed_ports == 2 and self._http_port and self._https_port:
      logging.debug("WPR: https_port=%s http_port=%s", self._https_port,
                    self._http_port)
      return True
    return False

  def _open_wpr_cmd_url(self, cmd: str):
    test_url = f"http://{self._host}:{self._http_port}/web-page-replay-{cmd}"
    return helper.urlopen(test_url, timeout=1)

  def stop(self, force_shutdown: bool = False) -> None:
    if self._process and not force_shutdown:
      self._shut_down()
    if self._log_file:
      self._log_file.close()
      self._log_file = None
    if self._process:
      helper.wait_and_kill(self._process, timeout=1)
    self._process = None

  def _shut_down(self) -> None:
    logging.info("WPR: shutting down recorder.")
    try:
      with self._open_wpr_cmd_url("command-exit"):
        pass
    except IOError as e:
      logging.debug("WPR: clean shut down failed: %s", e)


class WprRecorder(WprBase):
  NAME: str = "recorder"

  @property
  def cmd(self) -> TupleCmdArgs:
    return ("record",) + super().base_cmd_flags + (str(self._archive_path),)

  def _validate_archive_path(self, path: LocalPath) -> LocalPath:
    return cli_helper.parse_not_existing_path(path, "Wpr.go result archive")


class WprReplayServer(WprBase):
  NAME: str = "replay"

  def __init__(self,
               archive_path: LocalPath,
               bin_path: LocalPath,
               http_port: int = 0,
               https_port: int = 0,
               host: str = "127.0.0.1",
               inject_scripts: Optional[Iterable[LocalPath]] = None,
               key_file: Optional[LocalPath] = None,
               cert_file: Optional[LocalPath] = None,
               rules_file: Optional[LocalPath] = None,
               log_path: Optional[LocalPath] = None,
               fuzzy_url_matching: bool = True,
               serve_chronologically: bool = True,
               platform: Platform = PLATFORM):
    super().__init__(archive_path, bin_path, http_port, https_port, host,
                     inject_scripts, key_file, cert_file, log_path, platform)
    self._rules_file: Optional[LocalPath] = None
    if rules_file:
      self._rules_file = cli_helper.parse_non_empty_file_path(rules_file)
    self._fuzzy_url_matching: bool = fuzzy_url_matching
    self._serve_chronologically: bool = serve_chronologically

  def _validate_archive_path(self, path: LocalPath) -> LocalPath:
    return cli_helper.parse_non_empty_file_path(path, "WPR.go replay archive")

  @property
  def cmd(self) -> TupleCmdArgs:
    cmd = ("replay",) + super().base_cmd_flags
    if self._rules_file:
      cmd += (f"--rules_file={self._rules_file }",)
    if not self._fuzzy_url_matching:
      cmd += ("--disable_fuzzy_url_matching",)
    if self._serve_chronologically:
      cmd += ("--serve_response_in_chronological_sequence",)
    cmd += (str(self._archive_path),)
    return cmd
