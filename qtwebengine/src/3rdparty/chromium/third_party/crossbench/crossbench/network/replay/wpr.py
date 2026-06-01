# Copyright 2023 The Chromium Authors
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

from __future__ import annotations

import abc
import contextlib
import dataclasses
import logging
from typing import (TYPE_CHECKING, Final, Iterator, List, Mapping, Optional,
                    Tuple, Union)

from crossbench.flags.base import Flags
from crossbench.helper.path_finder import WprGoToolFinder
from crossbench.network.replay.base import GS_PREFIX, ReplayNetwork
from crossbench.network.replay.web_page_replay import WprReplayServer
from crossbench.parse import PathParser
from crossbench.path import check_hash
from crossbench.plt import PLATFORM, Platform

if TYPE_CHECKING:
  from crossbench.browsers.attributes import BrowserAttributes
  from crossbench.network.base import TrafficShaper
  from crossbench.path import AnyPath, LocalPath
  from crossbench.runner.groups.session import BrowserSessionRunGroup


# use value for pylint
assert GS_PREFIX

WPR_BASE_URL = "gs://chromium-telemetry/binary_dependencies"


@dataclasses.dataclass
class WPRCloudBinary:
  file_hash: str

  @property
  def url(self):
    return f"{WPR_BASE_URL}/wpr_go_{self.file_hash}"


# See third_party/catapult/telemetry/telemetry/binary_dependencies.json
WPR_PREBUILT_LOOKUP: Final[Mapping[Tuple[str, str], WPRCloudBinary]] = {
    ("android", "arm64"):
        WPRCloudBinary("129a66a1378dfcbb815596f66ca680728f77da36"),
    ("android", "arm32"):
        WPRCloudBinary("92ff5bdb9370b36d2844c2f018e2b7d9c3b8ed39"),
    ("android", "x64"):
        WPRCloudBinary("6caa467dc6bef92e1c34256f539f8ed8f26a2fe1"),
    # On arm64 ChromeOS, use the same binary as arm64 Linux.
    ("chromeos_ssh", "arm64"):
        WPRCloudBinary("129a66a1378dfcbb815596f66ca680728f77da36"),
    # On x64 ChromeOS, use the same binary as x64 Linux.
    ("chromeos_ssh", "x64"):
        WPRCloudBinary("6caa467dc6bef92e1c34256f539f8ed8f26a2fe1"),
    ("linux", "x64"):
        WPRCloudBinary("6caa467dc6bef92e1c34256f539f8ed8f26a2fe1"),
    ("macos", "arm64"):
        WPRCloudBinary("c68bd02b247e38a68a8e8ca154164fab75638e2e"),
    ("macos", "x64"):
        WPRCloudBinary("57443617185913f5e9af20e69a105419eb4cbea5"),
    ("win", "x64"):
        WPRCloudBinary("8b5310e99091991b949103b1edf39db45c7818f5"),
}


class WprReplayNetwork(ReplayNetwork):

  def __init__(self,
               archive: Union[LocalPath, str],
               traffic_shaper: Optional[TrafficShaper] = None,
               wpr_go_bin: Optional[LocalPath] = None,
               browser_platform: Platform = PLATFORM,
               persist_server: bool = False,
               inject_deterministic_script: bool = True):
    super().__init__(archive, traffic_shaper, browser_platform)
    self._server: Optional[WprReplayServer] = None
    self._tmp_dir: Optional[AnyPath] = None
    self._persist_server = persist_server
    self._inject_deterministic_script = inject_deterministic_script
    self._ensure_wpr_go(wpr_go_bin)

  def extra_flags(self, browser_attributes: BrowserAttributes) -> Flags:
    assert self.is_running, "Extra network flags are not valid"
    assert self._server
    if not browser_attributes.is_chromium_based:
      raise ValueError(
          "Only chromium-based browsers are supported for wpr replay.")
    # TODO: make ports configurable.
    extra_flags = super().extra_flags(browser_attributes)
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

  @abc.abstractmethod
  def _ensure_wpr_go(self, wpr_go_bin: Optional[LocalPath] = None):
    pass

  @abc.abstractmethod
  def _create_server(self, log_dir: LocalPath) -> WprReplayServer:
    pass

  @contextlib.contextmanager
  def open(self, session: BrowserSessionRunGroup) -> Iterator[ReplayNetwork]:
    with super().open(session):
      yield self

  def _ensure_server_started(self, session: BrowserSessionRunGroup):
    log_dir = session.browser_dir if self._persist_server else session.out_dir
    if not self._server or not self._persist_server:
      self._server = self._create_server(log_dir)
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
      if not self._persist_server and self._server:
        self._server.stop()

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

  @property
  def inject_deterministic_script(self) -> bool:
    return self._inject_deterministic_script

  def __str__(self) -> str:
    return f"WPR(archive={self.archive_path}, speed={self.traffic_shaper})"


class LocalWprReplayNetwork(WprReplayNetwork):

  def _ensure_wpr_go(self, wpr_go_bin: Optional[LocalPath] = None):
    if not wpr_go_bin:
      if local_wpr_go := WprGoToolFinder(self.host_platform).path:
        wpr_go_bin = self.host_platform.local_path(local_wpr_go)
    if not wpr_go_bin:
      raise RuntimeError(
          f"Could not find wpr.go binary on {self.host_platform}")
    if wpr_go_bin.suffix == ".go" and not self.host_platform.which("go"):
      raise ValueError(f"'go' binary not found on {self.host_platform}")
    self._wpr_go_bin: LocalPath = self.host_platform.local_path(
        PathParser.binary_path(wpr_go_bin, "wpr.go source"))

  @contextlib.contextmanager
  def open(self, session: BrowserSessionRunGroup) -> Iterator[ReplayNetwork]:
    with super().open(session):
      with self._forward_ports(session):
        yield self

  @contextlib.contextmanager
  def _forward_ports(self, session: BrowserSessionRunGroup) -> Iterator:
    browser_platform = session.browser_platform
    if not self._traffic_shaper.is_live or not browser_platform.is_remote:
      yield
      return
    http_port = self.http_port
    https_port = self.https_port
    logging.info("REMOTE PORT FORWARDING: %s <= %s", self.host_platform,
                 browser_platform)
    # TODO: create port-forwarder service that is shut down properly.
    # TODO: make ports configurable
    browser_platform.reverse_port_forward(http_port, http_port)
    browser_platform.reverse_port_forward(https_port, https_port)
    yield
    browser_platform.stop_reverse_port_forward(http_port)
    browser_platform.stop_reverse_port_forward(https_port)

  def _create_server(self, log_dir: LocalPath) -> WprReplayServer:
    inject_scripts: Optional[List[AnyPath]] = (
        None if self.inject_deterministic_script else [])
    return WprReplayServer(
        self.archive_path,
        self._wpr_go_bin,
        inject_scripts=inject_scripts,
        log_path=log_dir / "network.wpr.log",
        platform=self.host_platform)


class RemoteWprReplayNetwork(WprReplayNetwork):

  @classmethod
  def is_compatible(cls, platform: Platform) -> bool:
    return platform.is_android or platform.is_chromeos

  def _ensure_wpr_go(self, wpr_go_bin: Optional[LocalPath] = None):
    assert RemoteWprReplayNetwork.is_compatible(self.browser_platform)
    if wpr_go_bin:
      if wpr_go_bin.suffix == ".go":
        raise ValueError(f"Can't run .go files on {self.browser_platform}")
    else:
      wpr_go_bin = self._download_prebuilt_wpr()
    self._wpr_go_bin: LocalPath = self.host_platform.local_path(
        PathParser.binary_path(wpr_go_bin, "wpr.go binary"))

  def _download_prebuilt_wpr(self) -> LocalPath:
    wpr_cloud_binary = WPR_PREBUILT_LOOKUP[self.browser_platform.key]
    local_wpr_go_bin = (
        self.host_platform.local_cache_dir("wpr") /
        str(self.browser_platform.machine) / "wpr_go")
    if not check_hash(local_wpr_go_bin, wpr_cloud_binary.file_hash):
      self.host_platform.sh("gsutil", "cp", wpr_cloud_binary.url,
                            local_wpr_go_bin)
    assert check_hash(local_wpr_go_bin, wpr_cloud_binary.file_hash)

    return local_wpr_go_bin

  @contextlib.contextmanager
  def open(self, session: BrowserSessionRunGroup) -> Iterator[ReplayNetwork]:
    with self._remote_temp_dir(session):
      with super().open(session):
        yield self

  @contextlib.contextmanager
  def _remote_temp_dir(self, session: BrowserSessionRunGroup) -> Iterator:
    with session.browser_platform.TemporaryDirectory() as tmp_dir:
      self._tmp_dir = tmp_dir
      yield
      self._tmp_dir = None

  def _push_file(self, path: LocalPath) -> AnyPath:
    assert self._tmp_dir is not None
    remote_path = self._tmp_dir / path.name
    self.browser_platform.push(path, remote_path)
    return remote_path

  def _push_required_files(self) -> List[AnyPath]:
    host_platform = self.host_platform
    if local_wpr_go := WprGoToolFinder(host_platform).path:
      wpr_root = self.host_platform.local_path(local_wpr_go.parents[1])
    else:
      raise RuntimeError(f"Could not fine local wpr.go on {host_platform}")

    all_files: List[LocalPath] = [
        self._archive_path, wpr_root / "ecdsa_key.pem",
        wpr_root / "ecdsa_cert.pem", wpr_root / "deterministic.js"
    ]
    remote_files = [self._push_file(path) for path in all_files]

    remote_wpr_go_bin = self._push_file(self._wpr_go_bin)
    self.browser_platform.sh("chmod", "+x", remote_wpr_go_bin)

    return [remote_wpr_go_bin] + remote_files

  def _create_server(self, log_dir: LocalPath) -> WprReplayServer:
    wpr_go_bin, archive, key_file, cert_file, inject_script =\
        self._push_required_files()
    inject_scripts: List[AnyPath] = ([inject_script] if
                                     self.inject_deterministic_script else [])
    return WprReplayServer(
        archive_path=archive,
        bin_path=wpr_go_bin,
        key_file=key_file,
        cert_file=cert_file,
        inject_scripts=inject_scripts,
        log_path=log_dir / "network.wpr.log",
        platform=self.browser_platform)
