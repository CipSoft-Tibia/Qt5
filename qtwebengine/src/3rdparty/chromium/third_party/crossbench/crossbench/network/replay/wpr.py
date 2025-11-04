# Copyright 2023 The Chromium Authors
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

from __future__ import annotations

import contextlib
import logging
from typing import TYPE_CHECKING, Iterator, Optional, Union

from crossbench import cli_helper
from crossbench.flags.base import Flags
from crossbench.helper.path_finder import WprGoToolFinder
from crossbench.network.replay.base import GS_PREFIX, ReplayNetwork
from crossbench.network.replay.web_page_replay import WprReplayServer
from crossbench.plt import PLATFORM, Platform

if TYPE_CHECKING:
  from crossbench.browsers.browser import Browser
  from crossbench.network.base import TrafficShaper
  from crossbench.path import LocalPath
  from crossbench.runner.groups.session import BrowserSessionRunGroup


# use value for pylint
assert GS_PREFIX


class WprReplayNetwork(ReplayNetwork):

  def __init__(self,
               archive: Union[LocalPath, str],
               traffic_shaper: Optional[TrafficShaper] = None,
               wpr_go_bin: Optional[LocalPath] = None,
               browser_platform: Platform = PLATFORM,
               persist_server: bool = False):
    super().__init__(archive, traffic_shaper, browser_platform)
    if not wpr_go_bin:
      if local_wpr_go := WprGoToolFinder(self.runner_platform).path:
        wpr_go_bin = self.runner_platform.local_path(local_wpr_go)
    if not wpr_go_bin:
      raise RuntimeError(
          f"Could not find wpr.go binary on {self.runner_platform}")
    if wpr_go_bin.suffix == ".go" and not self.runner_platform.which("go"):
      raise ValueError(f"'go' binary not found on {self.runner_platform}")
    self._wpr_go_bin: LocalPath = self.runner_platform.local_path(
        cli_helper.parse_binary_path(wpr_go_bin, "wpr.go source"))
    self._server: Optional[WprReplayServer] = None
    self._persist_server = persist_server

  def __del__(self):
    if self._persist_server:
      logging.debug("Stopping WPR server")
      self._server.stop()

  def extra_flags(self, browser: Browser) -> Flags:
    assert self.is_running, "Extra network flags are not valid"
    assert self._server
    if not browser.attributes.is_chromium_based:
      raise ValueError(
          "Only chromium-based browsers are supported for wpr replay.")
    # TODO: make ports configurable.
    extra_flags = super().extra_flags(browser)
    # TODO: read this from wpr_public_hash.txt like in the recorder probe
    extra_flags["--ignore-certificate-errors-spki-list"] = (
        "PhrPvGIaAMmd29hj8BCZOq096yj7uMpRNHpn5PDxI6I=,"
        "2HcXCSKKJS0lEXLQEWhpHUfGuojiU0tiT5gOF9LP6IQ=")
    if self._traffic_shaper.is_live:
      # Only remap ports if we're not using the SOCKS proxy from the traffic
      # shaper.
      extra_flags["--host-resolver-rules"] = (
          f"MAP *:80 {self.host}:{self.http_port},"
          f"MAP *:443 {self.host}:{self.https_port},"
          "EXCLUDE localhost")

    return extra_flags

  @contextlib.contextmanager
  def open(self, session: BrowserSessionRunGroup) -> Iterator[ReplayNetwork]:
    with super().open(session):
      with self._forward_ports(session):
        yield self

  def _ensure_server_started(self, session: BrowserSessionRunGroup):
    log_dir = session.root_dir if self._persist_server else session.out_dir
    if not self._server or not self._persist_server:
      self._server = WprReplayServer(
          self.archive_path,
          self._wpr_go_bin,
          http_port=8080,
          https_port=8081,
          log_path=log_dir / "network.wpr.log",
          platform=self.runner_platform)
      logging.debug("Starting WPR server")
      self._server.start()
    else:
      # TODO: reset wpr server state for reuse
      logging.debug("WPR server already started")

  @contextlib.contextmanager
  def _open_replay_server(self, session: BrowserSessionRunGroup):
    self._ensure_server_started(session)

    try:
      yield self
    finally:
      if not self._persist_server:
        self._server.stop()

  @contextlib.contextmanager
  def _forward_ports(self, session: BrowserSessionRunGroup) -> Iterator:
    browser_platform = session.browser_platform
    if not self._traffic_shaper.is_live or not browser_platform.is_remote:
      yield
      return
    http_port = self.http_port
    https_port = self.https_port
    logging.info("REMOTE PORT FORWARDING: %s <= %s", self.runner_platform,
                 browser_platform)
    # TODO: create port-forwarder service that is shut down properly.
    # TODO: make ports configurable
    browser_platform.reverse_port_forward(http_port, http_port)
    browser_platform.reverse_port_forward(https_port, https_port)
    yield
    browser_platform.stop_reverse_port_forward(http_port)
    browser_platform.stop_reverse_port_forward(https_port)

  @property
  def http_port(self) -> int:
    assert self._server, "WPR is not running"
    return self._server.http_port

  @property
  def https_port(self) -> int:
    assert self._server, "WPR is not running"
    return self._server.https_port

  @property
  def host(self) -> str:
    assert self._server, "WPR is not running"
    return self._server.host

  def __str__(self) -> str:
    return f"WPR(archive={self.archive_path}, speed={self.traffic_shaper})"
