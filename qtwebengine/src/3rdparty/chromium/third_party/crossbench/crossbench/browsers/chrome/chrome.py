# Copyright 2023 The Chromium Authors
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

from __future__ import annotations

from crossbench.browsers.attributes import BrowserAttributes
from crossbench.browsers.chrome.helper import ChromePathMixin
from crossbench.browsers.chromium.chromium import Chromium


class Chrome(ChromePathMixin, Chromium):

  @property
  def attributes(self) -> BrowserAttributes:
    return BrowserAttributes.CHROME | BrowserAttributes.CHROMIUM_BASED
