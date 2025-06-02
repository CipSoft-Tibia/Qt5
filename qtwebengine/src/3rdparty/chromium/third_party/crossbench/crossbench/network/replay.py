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


class ReplayServer:

  def __init__(self, archive_path: pathlib.Path) -> None:
    self._archive_path = archive_path

  @contextlib.contextmanager
  def open(self, network: ReplayNetwork,
           browser: Browser) -> Iterator[ReplayServer]:
    del network, browser
    # TODO: implement
    yield self


class ReplayNetwork(Network):

  def __init__(self,
               archive_path: pathlib.Path,
               traffic_shaper: Optional[TrafficShaper] = None,
               platform: plt.Platform = plt.PLATFORM):
    super().__init__(traffic_shaper, platform)
    self._archive_path = archive_path
    self._replay_server = ReplayServer(archive_path)

  @contextlib.contextmanager
  def open(self, browser: Browser) -> Iterator[Network]:
    with self._replay_server.open(self, browser):
      with self._traffic_shaper.open(self, browser):
        yield self
