# Copyright 2023 The Chromium Authors
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

from __future__ import annotations

import abc
import contextlib
from typing import Iterator, Optional, TYPE_CHECKING

from crossbench import plt

if TYPE_CHECKING:
  from crossbench.browsers.browser import Browser


class TrafficShaper:

  @contextlib.contextmanager
  def open(self, network: Network, browser: Browser) -> Iterator[TrafficShaper]:
    del network, browser
    # TODO: implement port forwarding based on local server using ts_proxy
    yield self


class Network(abc.ABC):

  def __init__(self,
               traffic_shaper: Optional[TrafficShaper] = None,
               platform: plt.Platform = plt.PLATFORM) -> None:
    self._traffic_shaper = traffic_shaper or TrafficShaper()
    self._platform = platform

  @abc.abstractmethod
  @contextlib.contextmanager
  def open(self, browser: Browser) -> Iterator[Network]:
    # Dummy implementation to make pytype happy
    yield self
