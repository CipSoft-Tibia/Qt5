# Copyright 2023 The Chromium Authors
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

from __future__ import annotations

from typing import TYPE_CHECKING

from selenium import webdriver
from selenium.webdriver.chrome.options import Options as ChromeOptions
from selenium.webdriver.chrome.service import Service as ChromeService

from crossbench.browsers.attributes import BrowserAttributes
from crossbench.browsers.chromium.applescript import ChromiumAppleScript

from crossbench.browsers.chrome.helper import ChromePathMixin

if TYPE_CHECKING:
  from selenium.webdriver.chromium.webdriver import ChromiumDriver


class ChromeAppleScript(ChromePathMixin, ChromiumAppleScript):

  WEB_DRIVER_OPTIONS = ChromeOptions
  WEB_DRIVER_SERVICE = ChromeService

  @property
  def attributes(self) -> BrowserAttributes:
    return (BrowserAttributes.CHROME | BrowserAttributes.CHROMIUM_BASED
            | BrowserAttributes.APPLESCRIPT)

  def _create_driver(self, options, service) -> ChromiumDriver:
    return webdriver.Chrome(  # pytype: disable=wrong-keyword-args
        options=options, service=service)
