# Copyright 2023 The Chromium Authors
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

from __future__ import annotations

import contextlib
from typing import Iterator, TYPE_CHECKING

if TYPE_CHECKING:
  from crossbench.browsers.browser import Browser

from .base import Network


class LiveNetwork(Network):

  @contextlib.contextmanager
  def open(self, browser: Browser) -> Iterator[Network]:
    with self._traffic_shaper.open(self, browser):
      # TODO: implement
      yield self
