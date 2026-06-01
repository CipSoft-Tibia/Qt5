# Copyright 2022 The Chromium Authors
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

from __future__ import annotations

import abc
import atexit
import logging
import os
import time
import traceback
from typing import TYPE_CHECKING, Any, List, Optional, Sequence, cast

import selenium.common.exceptions
import urllib3
from selenium import webdriver
from selenium.webdriver.remote.remote_connection import RemoteConnection

from crossbench.browsers.attributes import BrowserAttributes
from crossbench.browsers.browser import Browser
from crossbench.probes.internal.browser.driver_log import BrowserDriverLogProbe
from crossbench.types import JsonDict

if TYPE_CHECKING:
  import datetime as dt

  from selenium.webdriver.common.timeouts import Timeouts

  from crossbench.browsers.settings import Settings
  from crossbench.env import HostEnvironment
  from crossbench.path import AnyPath, LocalPath
  from crossbench.runner.groups.session import BrowserSessionRunGroup


class DriverException(RuntimeError):
  """Wrapper for more readable error messages than the default
  WebDriver exceptions."""

  def __init__(self, msg: str, browser: Optional[Browser] = None) -> None:
    self._browser = browser
    self._msg = msg
    super().__init__(msg)

  def __str__(self) -> str:
    browser_prefix = ""
    if self._browser:
      browser_prefix = f"browser={self._browser}: "
    return f"{browser_prefix}{self._msg}"


class WebDriverBrowser(Browser, metaclass=abc.ABCMeta):
  _private_driver: webdriver.Remote
  _driver_path: Optional[AnyPath]
  _driver_pid: int
  _pid: int
  log_file: Optional[LocalPath]

  def __init__(self,
               label: str,
               path: Optional[AnyPath] = None,
               settings: Optional[Settings] = None):
    super().__init__(label, path, settings)
    self._driver_path = self._settings.driver_path
    self._driver_log_file: Optional[LocalPath] = None

  @property
  def attributes(self) -> BrowserAttributes:
    return BrowserAttributes.WEBDRIVER

  @property
  def driver_log_file(self) -> Optional[LocalPath]:
    return self._driver_log_file

  def validate_binary(self) -> None:
    super().validate_binary()
    self._driver_path = self.platform.absolute(self._find_driver())
    # TODO: support remote chromedriver as well
    assert self.host_platform.exists(self._driver_path), (
        f"Webdriver path '{self._driver_path}' does not exist")

  @abc.abstractmethod
  def _find_driver(self) -> AnyPath:
    pass

  @abc.abstractmethod
  def _validate_driver_version(self) -> None:
    pass

  def validate_env(self, env: HostEnvironment) -> None:
    super().validate_env(env)
    self._validate_driver_version()

  def start(self, session: BrowserSessionRunGroup) -> None:
    assert self._driver_path
    if timeout := self.http_request_timeout:
      logging.debug("Setting http request timeout to %s", timeout)
      RemoteConnection.set_timeout(timeout.total_seconds())
    try:
      self._private_driver = self._start_driver(session, self._driver_path)
    except selenium.common.exceptions.WebDriverException as e:
      msg = e.msg or "Could not create Webdriver session."
      raise DriverException(msg, self) from e
    self._is_running = True
    atexit.register(self.force_quit)
    self._find_driver_pid()
    self._set_driver_timeouts(session)
    self._setup_window()

  def _find_driver_pid(self) -> None:
    service = getattr(self._private_driver, "service", None)
    if not service:
      return
    self._driver_pid = service.process.pid
    candidates: List[int] = []
    for child in self.platform.process_children(self._driver_pid):
      if str(child["exe"]) == str(self.path):
        candidates.append(child["pid"])
    if len(candidates) == 1:
      self._pid = candidates[0]
    else:
      logging.debug(
          "Could not find unique browser process for webdriver: %s, got %s",
          self, candidates)

  def _set_driver_timeouts(self, session: BrowserSessionRunGroup) -> None:
    """Adjust the global webdriver timeouts if the runner has custom timeout
    unit values.
    If timing.has_no_timeout each value is set to SAFE_MAX_TIMEOUT_TIMEDELTA."""
    timing = session.timing
    if not timing.timeout_unit:
      return
    if timing.has_no_timeout:
      logging.info("Disabling webdriver timeouts")
    else:
      factor = timing.timeout_unit.total_seconds()
      if factor != 1.0:
        logging.info("Increasing webdriver timeouts by %fx", factor)
    timeouts: Timeouts = self._private_driver.timeouts
    if implicit_wait := getattr(timeouts, "implicit_wait", None):
      timeouts.implicit_wait = timing.timeout_timedelta(
          implicit_wait).total_seconds()
    if script := getattr(timeouts, "script", None):
      timeouts.script = timing.timeout_timedelta(script).total_seconds()
    if page_load := getattr(timeouts, "page_load", None):
      timeouts.page_load = timing.timeout_timedelta(page_load).total_seconds()
    self._private_driver.timeouts = timeouts

  def _setup_driver_log_file(self) -> LocalPath:
    log_file = self.log_file
    assert log_file, "Missing browser log file"
    self._driver_log_file = log_file.with_suffix(".driver.log")
    assert self._driver_log_file.name == BrowserDriverLogProbe.NAME, (
        f"Expected driver log file name {BrowserDriverLogProbe.NAME}, "
        f"but got: {self._driver_log_file}")
    return self._driver_log_file

  def _setup_window(self) -> None:
    # Force main window to foreground.
    self._private_driver.switch_to.window(
        self._private_driver.current_window_handle)
    if (self.viewport.is_headless or
        not self._private_driver.capabilities["setWindowRect"]):
      return
    if self.viewport.is_fullscreen:
      self._private_driver.fullscreen_window()
    elif self.viewport.is_maximized:
      self._private_driver.maximize_window()
    else:
      self._private_driver.set_window_position(self.viewport.x, self.viewport.y)
      self._private_driver.set_window_size(self.viewport.width,
                                           self.viewport.height)

  @abc.abstractmethod
  def _start_driver(self, session: BrowserSessionRunGroup,
                    driver_path: AnyPath) -> webdriver.Remote:
    pass

  def details_json(self) -> JsonDict:
    details: JsonDict = super().details_json()
    log = cast(JsonDict, details["log"])
    if self.driver_log_file:
      log["driver"] = os.fspath(self.driver_log_file)
    return details

  def show_url(self, url: str, target: Optional[str] = None) -> None:
    logging.debug("WebDriverBrowser.show_url(%s, %s)", url, target)
    try:
      if target in ("_self", None):
        # Do the navigation in the active tab.
        pass
      elif target == "_new_tab":
        self._private_driver.switch_to.new_window("tab")
      elif target == "_new_window":
        self._private_driver.switch_to.new_window("window")
      else:
        raise RuntimeError(f"unexpected target {target}")
      self._private_driver.get(url)
    except selenium.common.exceptions.WebDriverException as e:
      if msg := e.msg:
        self._wrap_webdriver_exception(e, msg, url)
      raise

  def switch_to_new_tab(self) -> None:
    self._private_driver.switch_to.new_window("tab")

  def screenshot(self, path: LocalPath) -> None:
    if not self._private_driver.get_screenshot_as_file(path.as_posix()):
      raise DriverException(
          f"Browser failed to get_screenshot_as_file to file '{path}'", self)

  def _wrap_webdriver_exception(
      self, e: selenium.common.exceptions.WebDriverException, msg: str,
      url: str) -> None:
    if "net::ERR_CONNECTION_REFUSED" in msg:
      raise DriverException(
          f"Browser failed to load URL={url}. The URL is likely unreachable.",
          self) from e
    if "net::ERR_INTERNET_DISCONNECTED" in msg:
      raise DriverException(
          f"Browser failed to load URL={url}. "
          f"The device is not connected to the internet.", self) from e

  def js(
      self,
      script: str,
      timeout: Optional[dt.timedelta] = None,
      arguments: Sequence[object] = ()
  ) -> Any:
    logging.debug("WebDriverBrowser.js() timeout=%s, script: %s", timeout,
                  script)
    assert self._is_running
    try:
      if timeout is not None:
        assert timeout.total_seconds() > 0, (
            f"timeout must be a positive number, got: {timeout}")
        self._private_driver.set_script_timeout(timeout.total_seconds())
      return self._private_driver.execute_script(script, *arguments)
    except selenium.common.exceptions.WebDriverException as e:
      # pylint: disable=raise-missing-from
      raise ValueError(f"Could not execute JS: {e.msg}")

  def close_all_tabs(self) -> None:
    try:
      all_handles = self._private_driver.window_handles
      for handle in all_handles:
        self._private_driver.switch_to.window(handle)
        self._private_driver.close()
    except (selenium.common.exceptions.InvalidSessionIdException,
            urllib3.exceptions.MaxRetryError) as e:
      logging.debug("%s: Got errors while closing all tabs: {%s}", self, e)

  def quit(self) -> None:
    assert self._is_running
    self.close_all_tabs()
    self.force_quit()

  def force_quit(self) -> None:
    if getattr(self, "_private_driver", None) is None or not self._is_running:
      return
    atexit.unregister(self.force_quit)
    logging.debug("WebDriverBrowser.force_quit()")
    try:
      try:
        # Close the current window.
        self._private_driver.close()
        time.sleep(0.1)
      except selenium.common.exceptions.NoSuchWindowException:
        # No window is good.
        pass
      except selenium.common.exceptions.InvalidSessionIdException:
        # Closing the last tab will close the session as well.
        return
      try:
        self._private_driver.quit()
      except selenium.common.exceptions.InvalidSessionIdException:
        return
      # Sometimes a second quit is needed, ignore any warnings there
      try:
        self._private_driver.quit()
      except Exception as e:  # pylint: disable=broad-except
        logging.debug("Driver raised exception on quit: %s\n%s", e,
                      traceback.format_exc())
      return
    except Exception as e:  # pylint: disable=broad-except
      logging.debug("Could not quit browser: %s\n%s", e, traceback.format_exc())
    finally:
      self._is_running = False


class RemoteWebDriver(WebDriverBrowser, Browser):
  """Represent a remote WebDriver that has already been started"""

  def __init__(self, label: str, driver: webdriver.Remote) -> None:
    super().__init__(label=label, path=None)
    self._private_driver = driver
    self.version: str = driver.capabilities["browserVersion"]
    self.major_version: int = int(self.version.split(".")[0])

  @property
  def type_name(self) -> str:
    return "remote"

  @property
  def attributes(self) -> BrowserAttributes:
    return BrowserAttributes.WEBDRIVER | BrowserAttributes.REMOTE

  def _validate_driver_version(self) -> None:
    pass

  def _extract_version(self) -> str:
    raise NotImplementedError()

  def _find_driver(self) -> LocalPath:
    raise NotImplementedError()

  def _start_driver(self, session: BrowserSessionRunGroup,
                    driver_path: AnyPath) -> webdriver.Remote:
    raise NotImplementedError()

  def setup_binary(self) -> None:
    pass

  def start(self, session: BrowserSessionRunGroup) -> None:
    # Driver has already been started. We just need to mark it as running.
    self._is_running = True
    self._setup_window()

  def quit(self) -> None:
    # External code that started the driver is responsible for shutting it down.
    self._is_running = False
