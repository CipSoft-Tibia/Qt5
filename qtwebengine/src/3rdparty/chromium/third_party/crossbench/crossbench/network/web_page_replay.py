# Copyright 2023 The Chromium Authors
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

from __future__ import annotations

import abc
import atexit
import logging
import pathlib
import re
import subprocess
import time
from typing import Iterable, Optional, TextIO, Tuple

from crossbench import cli_helper, helper, plt


_WPR_PORT_RE = re.compile(r".*Starting server on "
                          r"(?P<protocol>http|https)://"
                          r"(?P<host>[^:]+):"
                          r"(?P<port>\d+)")


class WprBase(abc.ABC):

  _key_file: pathlib.Path
  _cert_file: pathlib.Path

  def __init__(self,
               result_path: pathlib.Path,
               bin_path: pathlib.Path,
               http_port: int = 0,
               https_port: int = 0,
               host: str = "127.0.0.1",
               inject_scripts: Optional[Iterable[pathlib.Path]] = None,
               key_file: Optional[pathlib.Path] = None,
               cert_file: Optional[pathlib.Path] = None,
               log_path: Optional[pathlib.Path] = None,
               platform: plt.Platform = plt.PLATFORM):
    self._platform: plt.Platform = platform
    self._process: Optional[subprocess.Popen] = None
    self._log_path: Optional[pathlib.Path] = log_path
    self._log_file: Optional[TextIO] = None
    self._bin_path = cli_helper.parse_non_empty_file_path(bin_path)
    if not self._platform.which("go"):
      raise ValueError(f"'go' binary not available on {self._platform}")
    self._result_path = result_path
    if result_path.exists():
      raise ValueError(f"Wpr.go result archive exists already: '{result_path}'")

    if http_port == https_port:
      raise ValueError("http_port must be different from https_port, "
                       f"but got twice: {http_port}")
    self._http_port = http_port
    self._https_port = https_port

    self._host: str = host

    wpr_root: pathlib.Path = self._bin_path.parents[1]

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
    self._inject_scripts: Tuple[pathlib.Path, ...] = tuple(inject_scripts)

  @property
  def http_port(self) -> int:
    return self._http_port

  @property
  def https_port(self) -> int:
    return self._https_port

  @property
  def cert_file(self) -> pathlib.Path:
    return self._cert_file

  @property
  @abc.abstractmethod
  def cmd(self) -> Tuple[str, ...]:
    pass

  @property
  def base_cmd_flags(self) -> Tuple[str, ...]:
    cmd = (
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
    gp_cmd = (
        "go",
        "run",
        self._bin_path,
    ) + self.cmd

    try:
      if self._log_path:
        self._log_file = self._log_path.open("w")
      with helper.ChangeCWD(self._bin_path.parent):
        self._process = self._platform.popen(
            *gp_cmd, stdout=self._log_file, stderr=self._log_file)

      if not self._process:
        raise RuntimeError(f"Could not start {type(self).__name__}")

      atexit.register(self.stop)
      logging.info("WPR: waiting for startup")
      self._wait_for_startup()
      logging.info("WPR: Starting wpr.go recorder: DONE")

    except:
      self.stop()
      self._log_startup_error()
      raise

  def _log_startup_error(self):
    logging.error("WPR: Could not start %s", type(self).__name__)
    if not self._log_path or not self._log_path.exists():
      return
    logging.error("WPR: Check log files %s", self._log_path)
    try:
      with self._log_path.open() as f:
        log_lines = list(f.readlines())
        logging.error("  %s", "  ".join(log_lines[-4:]))
    except Exception as e:
      logging.debug("Got exception while reading wpr log file: %s", e)

  def _wait_for_startup(self) -> None:
    assert self._process and self._log_path
    with self._log_path.open("r") as log_file:
      while self._process.poll() is None:
        line = log_file.readline()
        if not line:
          time.sleep(0.1)
          continue
        if self._parse_wpr_log_line(line):
          break
    if self._process.poll():
      self._raise_startup_failure()
    with self._open_wpr_cmd_url("generate-200") as r:
      if r.status != 200:
        self._raise_startup_failure()

  def _raise_startup_failure(self) -> None:
    raise ValueError("Could not start wpr.go.\n"
                     f"See log for more details: {self._log_path}")

  def _parse_wpr_log_line(self, line: str) -> bool:
    if "Failed to start server on" in line:
      logging.error(line)
      raise ValueError(f"Could not start wpr.go server, address in use: {line}")
    line = line.strip()
    if match := _WPR_PORT_RE.match(line):
      protocol = match["protocol"].lower()
      port = int(match["port"])
      if protocol == "http":
        self._http_port = port
      elif protocol == "https":
        self._https_port = port
      else:
        logging.error("WPR: got invalid protocol: %s", line)
      self._host = match["host"]
      if not self._host:
        raise ValueError(f"WPR: could not parse host from: {line}")

    if self._http_port and self._https_port:
      logging.debug("WPR: https_port=%s http_port=%s", self._http_port,
                    self._https_port)
      return True
    return False

  def _open_wpr_cmd_url(self, cmd: str):
    test_url = f"http://{self._host}:{self._http_port}/web-page-replay-{cmd}"
    return helper.urlopen(test_url)

  def stop(self) -> None:
    if self._process:
      self._shut_down()
    if self._log_file:
      self._log_file.close()
      self._log_file = None
    if self._process:
      helper.wait_and_kill(self._process, timeout=5)
    self._process = None

  def _shut_down(self) -> None:
    logging.info("WPR: shutting down recorder.")
    try:
      with self._open_wpr_cmd_url("command-exit"):
        pass
    except IOError as e:
      logging.debug("WPR: clean shut down failed: %s", e)


class WprRecorder(WprBase):

  @property
  def cmd(self) -> Tuple[str, ...]:
    return ("record",) + super().base_cmd_flags + (str(self._result_path),)


class WprReplayServer(WprBase):

  def __init__(self,
               result_path: pathlib.Path,
               bin_path: pathlib.Path,
               http_port: int = 0,
               https_port: int = 0,
               host: str = "127.0.0.1",
               inject_scripts: Optional[Iterable[pathlib.Path]] = None,
               key_file: Optional[pathlib.Path] = None,
               cert_file: Optional[pathlib.Path] = None,
               rules_file: Optional[pathlib.Path] = None,
               log_path: Optional[pathlib.Path] = None,
               fuzzy_url_matching: bool = True,
               serve_chronologically: bool = True,
               platform: plt.Platform = plt.PLATFORM):
    super().__init__(result_path, bin_path, http_port, https_port, host,
                     inject_scripts, key_file, cert_file, log_path, platform)
    self._rules_file: Optional[pathlib.Path] = None
    if rules_file:
      self._rules_file = cli_helper.parse_non_empty_file_path(rules_file)
    self._fuzzy_url_matching: bool = fuzzy_url_matching
    self._serve_chronologically: bool = serve_chronologically

  @property
  def cmd(self) -> Tuple[str, ...]:
    cmd = ("replay",) + super().base_cmd_flags
    if self._rules_file:
      cmd += (f"--rules_file={self._rules_file }",)
    if not self._fuzzy_url_matching:
      cmd += ("--disable_fuzzy_url_matching",)
    if self._serve_chronologically:
      cmd += ("--serve_response_in_chronological_sequence",)
    cmd += (str(self._result_path),)
    return cmd
