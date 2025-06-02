# Copyright 2023 The Chromium Authors
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

from __future__ import annotations

import contextlib
import pathlib
from typing import Iterator, Optional, TYPE_CHECKING

from crossbench import plt

from .base import Network, TrafficShaper

if TYPE_CHECKING:
  from crossbench.browsers.browser import Browser


class LocalFileServer:

  def __init__(self,
               path: pathlib.Path,
               platform: Optional[plt.Platform] = plt.PLATFORM) -> None:
    self._path = path
    self._platform = platform

  @contextlib.contextmanager
  def open(self, network: LocalFileNetwork,
           browser: Browser) -> Iterator[LocalFileServer]:
    del network, browser
    yield self


class LocalFileNetwork(Network):

  def __init__(self,
               path: pathlib.Path,
               traffic_shaper: Optional[TrafficShaper] = None,
               platform: plt.Platform = plt.PLATFORM):
    super().__init__(traffic_shaper, platform)
    self._local_file_server = LocalFileServer(path, platform)

  @contextlib.contextmanager
  def open(self, browser: Browser) -> Iterator[Network]:
    with self._local_file_server.open(self, browser):
      with self._traffic_shaper.open(self, browser):
        yield self
