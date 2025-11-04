# Copyright 2023 The Chromium Authors
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

from __future__ import annotations

import logging
from typing import TYPE_CHECKING, List

import selenium.common.exceptions
from selenium import webdriver
from selenium.webdriver.chrome.options import Options as ChromeOptions
from selenium.webdriver.chrome.service import Service as ChromeService

from crossbench.browsers.attributes import BrowserAttributes
from crossbench.browsers.chrome.helper import ChromePathMixin
from crossbench.browsers.chromium.webdriver import (
    ChromiumWebDriver, ChromiumWebDriverAndroid, ChromiumWebDriverSsh,
    ChromiumWebDriverChromeOsSsh, build_chromedriver_instructions)
from crossbench.browsers.webdriver import DriverException

if TYPE_CHECKING:
  from selenium.webdriver.chromium.options import ChromiumOptions
  from selenium.webdriver.chromium.service import ChromiumService
  from selenium.webdriver.chromium.webdriver import ChromiumDriver


class ChromeWebDriver(ChromePathMixin, ChromiumWebDriver):

  WEB_DRIVER_OPTIONS = ChromeOptions
  WEB_DRIVER_SERVICE = ChromeService

  @property
  def attributes(self) -> BrowserAttributes:
    return (BrowserAttributes.CHROME | BrowserAttributes.CHROMIUM_BASED
            | BrowserAttributes.WEBDRIVER)

  def _create_driver(self, options: ChromiumOptions,
                     service: ChromiumService) -> ChromiumDriver:
    assert isinstance(options, ChromeOptions)
    assert isinstance(service, ChromeService)
    try:
      return webdriver.Chrome(  # pytype: disable=wrong-keyword-args
          options=options,
          service=service)
    except selenium.common.exceptions.WebDriverException as e:
      msg: List[str] = [f"Could not start WebDriver: {e.msg}"]
      if self.platform.is_android:
        msg += [
            f"Possibly missing chrome settings on {self.platform}.",
            "Please make sure to allow chrome-flags on "
            "non-rooted android devices:",
            "chrome://flags#enable-command-line-on-non-rooted-devices",
        ]
      if self.is_locally_compiled():
        msg.append(build_chromedriver_instructions(self.app_path.parent))
      msg_str = "\n".join(msg)
      logging.error(msg_str)
      raise DriverException(msg_str) from e


class ChromeWebDriverAndroid(ChromiumWebDriverAndroid, ChromeWebDriver):
  pass


class ChromeWebDriverSsh(ChromiumWebDriverSsh, ChromeWebDriver):
  pass


class ChromeWebDriverChromeOsSsh(ChromiumWebDriverChromeOsSsh, ChromeWebDriver):
  pass
