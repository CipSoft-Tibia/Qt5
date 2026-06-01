# Copyright 2023 The Chromium Authors
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

from __future__ import annotations

import contextlib
import logging
import re
from typing import TYPE_CHECKING, Iterator, Optional, Union
from urllib.parse import urlparse

from crossbench import exception
from crossbench import path as pth
from crossbench.helper.spinner import Spinner
from crossbench.network.base import Network
from crossbench.parse import PathParser

if TYPE_CHECKING:
  from crossbench import plt
  from crossbench.network.traffic_shaping.base import TrafficShaper
  from crossbench.path import LocalPath
  from crossbench.runner.groups.session import BrowserSessionRunGroup


GS_PREFIX = "gs://"
GSUTIL_LS_MD5_RE = re.compile(r"Hash \(md5\):\s*([A-Za-z0-9+/]+)=*")


class ReplayNetwork(Network):
  """ A network implementation that can be used to replay requests
  from a an archive."""

  def __init__(self,
               archive: Union[pth.LocalPath, str],
               traffic_shaper: Optional[TrafficShaper] = None,
               browser_platform: Optional[plt.Platform] = None):
    super().__init__(traffic_shaper, browser_platform)
    self._archive_path = self._ensure_archive(archive)

  @property
  def is_wpr(self) -> bool:
    return True

  @property
  def archive_path(self) -> LocalPath:
    return self._archive_path

  @contextlib.contextmanager
  def open(self, session: BrowserSessionRunGroup) -> Iterator[ReplayNetwork]:
    with super().open(session):
      with self._open_replay_server(session):
        with self._traffic_shaper.open(self, session):
          yield self

  @contextlib.contextmanager
  def _open_replay_server(self, session: BrowserSessionRunGroup):
    del session
    yield

  def _generate_filename(self, url: str) -> str:
    metadata = self.host_platform.sh_stdout("gsutil", "ls", "-L", url)
    if md5_search := GSUTIL_LS_MD5_RE.search(metadata):
      md5 = md5_search.group(1)
      safe_md5 = pth.safe_filename(md5)
      url_path = pth.AnyPosixPath(urlparse(url).path)
      return f"{url_path.stem}_{safe_md5}{url_path.suffix}"
    raise RuntimeError(f"Could not find md5 hash in gsutil output: {metadata}")

  def _download_gcloud_archive(self, url: str) -> LocalPath:
    with exception.annotate(f"Downloading {url}"), Spinner():
      local_path = (
          self.host_platform.local_cache_dir("wpr") /
          self._generate_filename(url))
      if local_path.is_file():
        logging.info("Found cached WPR archive: %s", local_path)
        return local_path
      logging.info("Downloading WPR archive from %s to %s", url, local_path)
      self.host_platform.sh("gsutil", "cp", url, local_path)
    return local_path

  def _ensure_archive(self, archive: Union[pth.LocalPath, str]) -> LocalPath:
    if isinstance(archive, str) and archive.startswith(GS_PREFIX):
      return self._download_gcloud_archive(url=archive)
    return PathParser.existing_file_path(archive).resolve()
