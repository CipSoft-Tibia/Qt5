# Copyright 2023 The Chromium Authors
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

from __future__ import annotations

from typing import TYPE_CHECKING

from crossbench.browsers.attributes import BrowserAttributes
from crossbench.env import HostEnvironment

from crossbench.probes.probe import Probe

if TYPE_CHECKING:
  from crossbench.browsers.browser import Browser


class ChromiumProbe(Probe):

  def validate_browser(self, env: HostEnvironment, browser: Browser) -> None:
    super().validate_browser(env, browser)
    self.expect_browser(browser, BrowserAttributes.CHROMIUM_BASED)
