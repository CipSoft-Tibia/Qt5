# Copyright 2023 The Chromium Authors
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

from __future__ import annotations

from crossbench.browsers.attributes import BrowserAttributes
from crossbench.browsers.chrome.paths import ChromePathMixin
from crossbench.browsers.chromium_based.chromium_based import ChromiumBased


class Chrome(ChromePathMixin, ChromiumBased):

  @property
  def attributes(self) -> BrowserAttributes:
    return BrowserAttributes.CHROME | BrowserAttributes.CHROMIUM_BASED
