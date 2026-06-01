# Copyright 2024 The Chromium Authors
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

from __future__ import annotations

from crossbench.browsers.attributes import BrowserAttributes
from crossbench.browsers.chromium.paths import ChromiumPathMixin
from crossbench.browsers.chromium_based.chromium_based import ChromiumBased


class Chromium(ChromiumPathMixin, ChromiumBased):

  @property
  def attributes(self) -> BrowserAttributes:
    return BrowserAttributes.CHROMIUM | BrowserAttributes.CHROMIUM_BASED

  @property
  def type_name(self) -> str:
    return "chromium"
