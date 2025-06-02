# Copyright 2023 The Chromium Authors
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

from __future__ import annotations

import logging
import pathlib
from typing import TYPE_CHECKING, Optional

from selenium import webdriver
from selenium.webdriver.safari.options import Options as SafariOptions
from selenium.webdriver.safari.service import Service as SafariService

from crossbench import exception, helper
from crossbench.browsers.splash_screen import SplashScreen
from crossbench.browsers.webdriver import WebDriverBrowser

from .safari import Safari, find_safaridriver

if TYPE_CHECKING:
  from crossbench import plt
  from crossbench.browsers.splash_screen import SplashScreen
  from crossbench.browsers.viewport import Viewport
  from crossbench.flags import Flags
  from crossbench.network.base import Network
  from crossbench.runner.groups import BrowserSessionRunGroup
  from crossbench.runner.runner import Runner


class SafariWebDriver(WebDriverBrowser, Safari):

  def __init__(
      self,
      label: str,
      path: pathlib.Path,
      flags: Optional[Flags.InitialDataType] = None,
      js_flags: Optional[Flags.InitialDataType] = None,
      cache_dir: Optional[pathlib.Path] = None,
      type: str = "safari",  # pylint: disable=redefined-builtin
      network: Optional[Network] = None,
      driver_path: Optional[pathlib.Path] = None,
      viewport: Optional[Viewport] = None,
      splash_screen: Optional[SplashScreen] = None,
      platform: Optional[plt.MacOSPlatform] = None):
    super().__init__(label, path, flags, js_flags, cache_dir, type, network,
                     driver_path, viewport, splash_screen, platform)
    assert self.platform.is_macos

  def clear_cache(self, runner: Runner) -> None:
    # skip the default caching, and only do it after launching the browser
    # via selenium.
    pass

  def _find_driver(self) -> pathlib.Path:
    return find_safaridriver(self.path)

  def _start_driver(self, session: BrowserSessionRunGroup,
                    driver_path: pathlib.Path) -> webdriver.Safari:
    assert not self._is_running
    logging.info("STARTING BROWSER: browser: %s driver: %s", self.path,
                 driver_path)

    options: SafariOptions = self._get_driver_options(session)
    session.setup_selenium_options(options)
    self._force_clear_cache(session)

    service = SafariService(executable_path=str(driver_path))
    driver_kwargs = {"service": service, "options": options}

    if webdriver.__version__ == '4.1.0':
      # Manually inject desired options for older selenium versions
      # (currently fixed version from vpython3).
      self._legacy_settings(options, driver_kwargs)

    driver = webdriver.Safari(**driver_kwargs)

    assert driver.session_id, "Could not start webdriver"
    logs = (
        pathlib.Path("~/Library/Logs/com.apple.WebDriver/").expanduser() /
        driver.session_id)
    all_logs = list(logs.glob("safaridriver*"))
    if all_logs:
      self.log_file = all_logs[0]
      assert self.log_file.is_file()
    return driver

  def _legacy_settings(self, options, driver_kwargs) -> None:
    logging.debug("SafariDriver: using legacy capabilities")
    options.binary_location = str(self.path)
    driver_kwargs["desired_capabilities"] = options.to_capabilities()

  def _force_clear_cache(self, session: BrowserSessionRunGroup) -> None:
    del session
    with exception.annotate("Clearing Browser Cache"):
      self._clear_cache()
      self.platform.exec_apple_script(f"""
        tell application "{self.app_path}" to quit """)

  def _get_driver_options(self,
                          session: BrowserSessionRunGroup) -> SafariOptions:
    options = SafariOptions()
    # Don't wait for document-ready.
    options.set_capability("pageLoadStrategy", "eager")

    args = self._get_browser_flags_for_session(session)
    for arg in args:
      options.add_argument(arg)

    # TODO: Conditionally enable detailed browser logging
    # options.set_capability("safari:diagnose", "true")
    if "Technology Preview" in self.app_name:
      options.set_capability("browserName", "Safari Technology Preview")
      options.use_technology_preview = True
    return options

  def _check_driver_version(self) -> None:
    # The bundled driver is always ok
    assert self._driver_path
    for parent in self._driver_path.parents:
      if parent == self.path.parent:
        return
    version = self.platform.sh_stdout(self._driver_path, "--version")
    assert str(self.major_version) in version, (
        f"safaridriver={self._driver_path} version='{version}' "
        f" doesn't match safari version={self.major_version}")

  def _setup_window(self) -> None:
    super()._setup_window()
    self.platform.exec_apple_script(f"""
        tell application "{self.app_name}"
          activate
        end tell""")

  def quit(self, runner: Runner) -> None:
    super().quit(runner)
    # Safari needs some additional push to quit properly
    self.platform.exec_apple_script(f"""
        tell application "{self.app_name}"
          quit
        end tell""")


class SafariWebdriverIOS(SafariWebDriver):
  # TODO(cbruni): implement iOS platform
  def _start_driver(self, session: BrowserSessionRunGroup,
                    driver_path: pathlib.Path) -> webdriver.Safari:
    # safaridriver for iOS seems to be brittle for starting up, we give it
    # several chances to start up.
    for timeout in helper.wait_with_backoff(
        helper.WaitRange(min=2, timeout=15)):
      try:
        return super()._start_driver(session, driver_path)
      except Exception as e:  # pylint: disable=disable=broad-except
        logging.warning("SafariWebDriver: startup failed, retrying (%s)",
                        timeout)
        logging.debug("SafariWebDriver: startup error %s", e)
    return super()._start_driver(session, driver_path)

  def _get_driver_options(self,
                          session: BrowserSessionRunGroup) -> SafariOptions:
    options = super()._get_driver_options(session)
    desired_cap = {
        # "browserName": "Safari",
        # "browserVersion": "17.0.3", # iOS version
        # "safari:deviceType": "iPhone",
        # "safari:deviceName": "XXX's iPhone",
        # "safari:deviceUDID": "...",
        "platformName": "iOS",
        "safari:initialUrl": "about:blank",
        "safari:openLinksInBackground": True,
        "safari:allowPopups": True,
    }
    for key, value in desired_cap.items():
      options.set_capability(key, value)
    return options

  def _setup_window(self) -> None:
    pass

  def _force_clear_cache(self, session: BrowserSessionRunGroup) -> None:
    pass

  def quit(self, runner: Runner) -> None:
    self._driver.close()
    self.platform.sleep(1.0)
    self._driver.quit()
    self.force_quit()
