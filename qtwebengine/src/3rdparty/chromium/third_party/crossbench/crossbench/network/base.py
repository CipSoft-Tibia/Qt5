# Copyright 2023 The Chromium Authors
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

from __future__ import annotations

import abc
import contextlib
from typing import TYPE_CHECKING, Iterator, Optional

from crossbench import plt
from crossbench.flags.base import Flags
from crossbench.network.traffic_shaping.base import (NoTrafficShaper,
                                                     TrafficShaper)

if TYPE_CHECKING:
  from crossbench.browsers.browser import Browser
  from crossbench.runner.groups.session import BrowserSessionRunGroup


class Network(abc.ABC):

  def __init__(self,
               traffic_shaper: Optional[TrafficShaper] = None,
               browser_platform: plt.Platform = plt.PLATFORM) -> None:
    self._traffic_shaper = traffic_shaper or NoTrafficShaper(browser_platform)
    self._browser_platform = browser_platform
    self._runner_platform = browser_platform.host_platform
    self._is_running: bool = False

  @property
  def traffic_shaper(self) -> TrafficShaper:
    return self._traffic_shaper

  @property
  def browser_platform(self) -> plt.Platform:
    return self._browser_platform

  @property
  def runner_platform(self) -> plt.Platform:
    return self._runner_platform

  @property
  def is_running(self) -> bool:
    return self._is_running

  @property
  def is_live(self) -> bool:
    """Return True if the network is the default live/direct connection, as
    opposed to a replay network or local file server."""
    return False

  @property
  def http_port(self) -> Optional[int]:
    """HTTP port for non-live server-based networks."""
    return None

  @property
  def https_port(self) -> Optional[int]:
    """HTTPS port for non-live server-based networks."""
    return None

  @property
  def host(self) -> Optional[str]:
    """Host for non-live server-based networks."""
    return None

  def extra_flags(self, browser: Browser) -> Flags:
    assert self.is_running, "Network is not running."
    return self.traffic_shaper.extra_flags(browser)

  @contextlib.contextmanager
  def open(self, session: BrowserSessionRunGroup) -> Iterator[Network]:
    del session
    assert not self._is_running, "Cannot start network more than once."
    self._is_running = True
    try:
      yield self
    finally:
      self._is_running = False
