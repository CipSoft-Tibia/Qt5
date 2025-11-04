# Copyright 2023 The Chromium Authors
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

# Modified from chrome's catapult project.

from __future__ import annotations

import atexit
import contextlib
import locale
import logging
import os
import re
import shlex
import signal
import subprocess
import sys
from typing import IO, TYPE_CHECKING, Iterator, List, Optional, Union

from crossbench import cli_helper, helper
from crossbench.flags.base import Flags
from crossbench.helper.path_finder import TsProxyFinder
from crossbench.network.traffic_shaping.base import TrafficShaper

if TYPE_CHECKING:
  from crossbench.browsers.browser import Browser
  from crossbench.network.base import Network
  from crossbench.path import LocalPath, RemotePath
  from crossbench.plt.base import ListCmdArgs, Platform
  from crossbench.runner.groups.session import BrowserSessionRunGroup

fnctl = None
try:
  import fcntl
except ModuleNotFoundError as not_found:
  logging.debug("No fcntl support %s", not_found)



class TsProxyServerError(Exception):
  """Catch-all exception for tsProxy Server."""


_PORT_RE = re.compile(r"Started Socks5 proxy server on "
                      r"(?P<host>[^:]*):"
                      r"(?P<port>\d+)")
DEFAULT_TIMEOUT = 5


def parse_ts_socks_proxy_port(output_line):
  if match := _PORT_RE.match(output_line):
    return int(match.group("port"))
  return None


# TODO: improve and double check
TRAFFIC_SETTINGS = {
    "3G-slow": {
        "rtt_ms": 400,
        "in_kbps": 400,
        "out_kbps": 400,
    },
    "3G-regular": {
        "rtt_ms": 300,
        "in_kbps": 1600,
        "out_kbps": 768,
    },
    "3G-fast": {
        "rtt_ms": 150,
        "in_kbps": 1600,
        "out_kbps": 768,
    },
    "4G": {
        "rtt_ms": 170,
        "in_kbps": 9000,
        "out_kbps": 9000,
    },
}


class TsProxyServer:
  """
  TsProxy provides basic latency, download and upload traffic shaping. This
  class provides a programming API to the tsproxy script in
  catapult/third_party/tsproxy/tsproxy.py

  This class can be used as a context manager.
  """

  def __init__(self,
               ts_proxy_path: LocalPath,
               host: Optional[str] = None,
               socks_proxy_port: Optional[int] = None,
               http_port: Optional[int] = None,
               https_port: Optional[int] = None,
               rtt_ms: Optional[int] = None,
               in_kbps: Optional[int] = None,
               out_kbps: Optional[int] = None,
               window: Optional[int] = None,
               verbose: bool = True):
    self._proc: Optional[TsProxyProcess] = None
    self._ts_proxy_path = cli_helper.parse_existing_file_path(ts_proxy_path)
    self._socks_proxy_port = socks_proxy_port
    self._host = host
    self._http_port = http_port
    self._https_port = https_port
    self._rtt_ms = rtt_ms
    self._in_kbps = in_kbps
    self._out_kbps = out_kbps
    self._window = window
    self._verbose = verbose
    self.verify_ports(http_port, https_port)

  @classmethod
  def verify_ports(cls,
                   http_port: Optional[int] = None,
                   https_port: Optional[int] = None) -> None:
    if https_port and not bool(http_port):
      raise ValueError(f"Got https_port={https_port} without a http port")
    if http_port is not None and http_port == https_port:
      raise ValueError("http_port and https_port must be different, "
                       f"got {https_port} twice.")
    if http_port is not None:
      cli_helper.parse_port(http_port, "http_port")
    if https_port is not None:
      cli_helper.parse_port(https_port, "https_port")

  @property
  def is_running(self) -> bool:
    return self._proc is not None

  def set_traffic_settings(self,
                           rtt_ms: Optional[int] = None,
                           in_kbps: Optional[int] = None,
                           out_kbps: Optional[int] = None,
                           window: Optional[int] = None,
                           timeout=DEFAULT_TIMEOUT) -> None:
    assert self._proc, "ts_proxy is not running."
    self._proc.set_traffic_settings(rtt_ms, in_kbps, out_kbps, window, timeout)

  @property
  def socks_proxy_port(self) -> int:
    assert self._proc, "ts_proxy is not running."
    return self._proc.socks_proxy_port

  @property
  def ts_proxy_path(self) -> LocalPath:
    return self._ts_proxy_path

  @property
  def rtt_ms(self) -> Optional[int]:
    return self._rtt_ms

  @property
  def in_kbps(self) -> Optional[int]:
    return self._in_kbps

  @property
  def out_kbps(self) -> Optional[int]:
    return self._out_kbps

  @property
  def window(self) -> Optional[int]:
    return self._window

  def start(self) -> None:
    assert not self._proc, "ts_proxy is already running."
    self._proc = TsProxyProcess(self._ts_proxy_path, self._host,
                                self._socks_proxy_port, self._http_port,
                                self._https_port, self._rtt_ms, self._in_kbps,
                                self._out_kbps, self._window, self._verbose)
    atexit.register(self.stop)

  def stop(self) -> Optional[str]:
    if not self._proc:
      logging.debug("TsProxy: Attempting to stop server that is not running.")
      return None
    assert self._proc
    err = self._proc.stop()
    self._proc = None
    return err

  def __enter__(self):
    self.start()
    return self

  def __exit__(self, unused_exc_type, unused_exc_val, unused_exc_tb):
    self.stop()


class TsProxyProcess:
  """Separate wrapper around the ts_proxy to simplify pytype testing."""

  def __init__(self,
               ts_proxy_path: LocalPath,
               host: Optional[str] = None,
               socks_proxy_port: Optional[int] = None,
               http_port: Optional[int] = None,
               https_port: Optional[int] = None,
               rtt_ms: Optional[int] = None,
               in_kbps: Optional[int] = None,
               out_kbps: Optional[int] = None,
               window: Optional[int] = None,
               verbose: bool = False,
               timeout: Union[int, float] = DEFAULT_TIMEOUT) -> None:
    """Start TsProxy server and verify that it started."""
    cmd: ListCmdArgs = [
        sys.executable,
        ts_proxy_path,
    ]
    self._socks_proxy_port: Optional[int] = socks_proxy_port
    self._initial_socks_proxy_port: Optional[int] = socks_proxy_port
    if not socks_proxy_port:
      # Use port 0 so tsproxy picks a random available port.
      cmd.append("--port=0")
    else:
      cmd.append(f"--port={socks_proxy_port}")
    if verbose:
      cmd.append("--verbose")
    self._in_kbps: Optional[int] = in_kbps
    if in_kbps:
      cmd.append(f"--inkbps={in_kbps}")
    self._out_kbps: Optional[int] = out_kbps
    if out_kbps:
      cmd.append(f"--outkbps={out_kbps}")
    self._window: Optional[int] = window
    if window:
      cmd.append(f"--window={window}")
    self._rtt_ms: Optional[int] = rtt_ms
    if rtt_ms:
      cmd.append(f"--rtt={rtt_ms}")
    self._host: Optional[str] = host
    if host:
      cmd.append(f"--desthost={host}")
    self._http_port: Optional[int] = http_port
    self._https_port: Optional[int] = https_port
    TsProxyServer.verify_ports(http_port, https_port)
    mapports = []
    if https_port:
      mapports.append(f"443:{https_port}")
    if http_port:
      mapports.append(f"*:{http_port}")
    cmd.append(f"--mapports={','.join(mapports)}")
    logging.info("TsProxy: commandline: %s", shlex.join(map(str, cmd)))
    self._verify_default_encoding()
    # In python3 universal_newlines forces subprocess to encode/decode,
    # allowing per-line buffering.
    proc = subprocess.Popen(  # pylint: disable=consider-using-with
        cmd,
        stdout=subprocess.PIPE,
        stdin=subprocess.PIPE,
        # stderr=subprocess.PIPE,
        bufsize=1,
        universal_newlines=True)
    assert proc and proc.stdout and proc.stdin, "Could not start ts_proxy"
    self._proc = proc
    if stdout := proc.stdout:
      self._stdout: IO[str] = stdout
    else:
      raise RuntimeError("Missing stdout")
    if stdin := proc.stdin:
      self._stdin: IO[str] = stdin
    else:
      raise RuntimeError("Missing stdin")
    if fcntl:  # pylint: disable=using-constant-test
      self._setup_non_blocking_io()
    self._wait_for_startup(timeout)

  def _setup_non_blocking_io(self) -> None:
    logging.debug("TsProxy: fcntl is supported, trying to set "
                  "non blocking I/O for the ts_proxy process")
    fd = self._stdout.fileno()
    fl = fcntl.fcntl(fd, fcntl.F_GETFL)
    fcntl.fcntl(fd, fcntl.F_SETFL, fl | os.O_NONBLOCK)  # pylint: disable=no-member

  @property
  def socks_proxy_port(self) -> int:
    if self._socks_proxy_port is None:
      raise RuntimeError("ts_proxy didn't start")
    return self._socks_proxy_port # pytype: disable=bad-return-type

  def _verify_default_encoding(self) -> None:
    # In python3 subprocess handles encoding/decoding; this warns if it won't
    # be UTF-8.
    encoding = locale.getpreferredencoding()
    if encoding != "UTF-8":
      logging.warning("Decoding will use %s instead of UTF-8", encoding)

  def _wait_for_startup(self, timeout: Union[int, float]) -> None:
    for _ in helper.wait_with_backoff(timeout):
      if self._has_started():
        logging.info("TsProxy: port=%i", self._socks_proxy_port)
        return
    if err := self.stop():
      logging.error("TsProxy: Error stopping WPR server:\n%s", err)
    raise TsProxyServerError(
        f"Starting tsproxy timed out after {timeout} seconds")

  def _has_started(self) -> bool:
    if self._proc.poll() is not None:
      return False
    self._stdout.flush()
    output_line = self._read_line_ts_proxy_stdout(timeout=5)
    if not output_line:
      return False
    logging.debug("TsProxy: output: %s", output_line)
    port = parse_ts_socks_proxy_port(output_line)
    self._socks_proxy_port = cli_helper.parse_port(port, "socks_proxy_port")
    return True

  def _read_line_ts_proxy_stdout(self, timeout: Union[int, float]) -> str:
    for _ in helper.wait_with_backoff(timeout):
      try:
        return self._stdout.readline().strip()
      except IOError as io_error:
        logging.debug("TsProxy: Error while reading tsproxy line: %s", io_error)
    return ""

  def _send_command(self,
                    command: str,
                    timeout: Union[int, float] = DEFAULT_TIMEOUT) -> None:
    logging.debug("TsProxy: Sending command to ts_proxy_server: %s", command)
    self._stdin.write(f"{command}\n")
    command_output = self._wait_for_status_response(timeout)
    success = "OK" in command_output
    logging.log(logging.DEBUG if success else logging.ERROR,
                "TsProxy: output:\n%s", "\n".join(command_output))
    if not success:
      raise TsProxyServerError(f"Failed to execute command: {command}")

  def _wait_for_status_response(self, timeout: Union[int, float]) -> List[str]:
    logging.debug("TsProxy: waiting for status response")
    command_output = []
    for _ in helper.wait_with_backoff(timeout):
      self._stdin.flush()
      self._stdout.flush()
      last_output = self._read_line_ts_proxy_stdout(timeout)
      command_output.append(last_output)
      if last_output in ("OK", "ERROR"):
        break
    return command_output

  def set_traffic_settings(self,
                           rtt_ms: Optional[int] = None,
                           in_kbps: Optional[int] = None,
                           out_kbps: Optional[int] = None,
                           window: Optional[int] = None,
                           timeout=DEFAULT_TIMEOUT) -> None:
    if rtt_ms is not None and self._rtt_ms != rtt_ms:
      assert rtt_ms >= 0, f"Invalid rtt value: {rtt_ms}"
      self._send_command(f"set rtt {rtt_ms}", timeout)
      self._rtt_ms = rtt_ms

    if in_kbps is not None and self._in_kbps != in_kbps:
      assert in_kbps >= 0, f"Invalid in_kbps value: {in_kbps}"
      self._send_command(f"set inkbps {in_kbps}", timeout)
      self._in_kbps = in_kbps

    if out_kbps is not None and self._out_kbps != out_kbps:
      assert out_kbps >= 0, f"Invalid out_kbps value: {out_kbps}"
      self._send_command(f"set outkbps {out_kbps}", timeout)
      self._out_kbps = out_kbps

    if window is not None and self._window != window:
      assert window >= 0, f"Invalid window value: {window}"
      self._send_command(f"set window {window}", timeout)
      self._window = window

  def stop(self) -> Optional[str]:
    self._send_command("exit")
    helper.wait_and_kill(self._proc, signal=signal.SIGINT)
    _, err = self._proc.communicate()
    self._socks_proxy_port = self._initial_socks_proxy_port
    return err


class TsProxyTrafficShaper(TrafficShaper):

  def __init__(self,
               browser_platform: Platform,
               ts_proxy_path: Optional[RemotePath] = None,
               rtt_ms: Optional[int] = None,
               in_kbps: Optional[int] = None,
               out_kbps: Optional[int] = None,
               window: Optional[int] = None):
    super().__init__(browser_platform)
    if not ts_proxy_path:
      if maybe_ts_proxy_path := TsProxyFinder(self.runner_platform).path:
        ts_proxy_path = self.runner_platform.local_path(maybe_ts_proxy_path)
    if not ts_proxy_path:
      raise RuntimeError(
          f"Could not find ts_proxy script on {self.runner_platform}")
    # Early instantiation to validate inputs.
    self._ts_proxy = TsProxyServer(
        self.runner_platform.local_path(ts_proxy_path),
        rtt_ms=rtt_ms,
        in_kbps=in_kbps,
        out_kbps=out_kbps,
        window=window)
    # TODO: support custom name
    self._name = "tsproxy"

  @contextlib.contextmanager
  def open(self, network: Network,
           session: BrowserSessionRunGroup) -> Iterator[TrafficShaper]:
    if not network.is_live:
      self._ts_proxy = self._create_remapping_ts_proxy(network)

    with super().open(network, session):
      logging.debug("Starting TS Proxy")
      with self._ts_proxy:
        with self._forward_ports(network, session):
          yield self

  def _create_remapping_ts_proxy(self, network) -> TsProxyServer:
    return TsProxyServer(
        self._ts_proxy.ts_proxy_path,
        rtt_ms=self._ts_proxy.rtt_ms,
        in_kbps=self._ts_proxy.in_kbps,
        out_kbps=self._ts_proxy.out_kbps,
        window=self._ts_proxy.window,
        host=network.host,
        http_port=network.http_port,
        https_port=network.https_port)

  @contextlib.contextmanager
  def _forward_ports(self, network: Network,
                     session: BrowserSessionRunGroup) -> Iterator:
    del network
    browser_platform = session.browser_platform
    ts_proxy_port = self._ts_proxy.socks_proxy_port
    # TODO; remap network port for remote browsers or when ports are occupied
    # already.
    if browser_platform.is_remote:
      browser_platform.reverse_port_forward(ts_proxy_port, ts_proxy_port)
    yield
    if browser_platform.is_remote:
      browser_platform.stop_reverse_port_forward(ts_proxy_port)

  def extra_flags(self, browser: Browser) -> Flags:
    if not browser.attributes.is_chromium_based:
      raise ValueError(
          "Only chromium-based browsers are supported with ts_proxy.")
    # TODO: support port forwarding to remote device
    assert browser.attributes.is_local, "Only local browsers supported for now"
    assert self.is_running, "TrafficShaper is not running."
    assert self._ts_proxy.socks_proxy_port, "ts_proxy is not running"
    return Flags({
        "--proxy-server":
            f"socks://127.0.0.1:{self._ts_proxy.socks_proxy_port}",
        "--proxy-bypass-list":
            "<-loopback>"
    })

  def __str__(self) -> str:
    return self._name
