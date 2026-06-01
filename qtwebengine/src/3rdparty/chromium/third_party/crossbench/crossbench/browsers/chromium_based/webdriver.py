# Copyright 2024 The Chromium Authors
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

from __future__ import annotations

import abc
import datetime as dt
import logging
import os
from typing import TYPE_CHECKING, Any, Iterable, List, Optional, Sequence, Type, Tuple, cast

from selenium.webdriver.chromium.options import ChromiumOptions
from selenium.webdriver.chromium.service import ChromiumService
from selenium.webdriver.chromium.webdriver import ChromiumDriver

from crossbench import path as pth
from crossbench.browsers.attributes import BrowserAttributes
from crossbench.browsers.chromium import helper
from crossbench.browsers.chromium.driver_finder import (ChromeDriverFinder,
                                                        DriverNotFoundError)
from crossbench.browsers.chromium.version import (ChromeDriverVersion,
                                                  ChromiumVersion)
from crossbench.browsers.chromium_based.chromium_based import ChromiumBased
from crossbench.browsers.webdriver import WebDriverBrowser
from crossbench.flags.base import FlagsT
from crossbench.flags.chrome import ChromeFlags
from crossbench.helper import wait

if TYPE_CHECKING:
  import re

  from selenium import webdriver

  from crossbench.runner.groups.session import BrowserSessionRunGroup


class ChromiumBasedWebDriver(
    WebDriverBrowser, ChromiumBased, metaclass=abc.ABCMeta):

  WEB_DRIVER_OPTIONS: Type[ChromiumOptions] = ChromiumOptions
  WEB_DRIVER_SERVICE: Type[ChromiumService] = ChromiumService
  UNSUPPORTED_FLAGS: Tuple[str, ...] = ()

  @property
  def attributes(self) -> BrowserAttributes:
    return (BrowserAttributes.CHROMIUM | BrowserAttributes.CHROMIUM_BASED
            | BrowserAttributes.WEBDRIVER)

  def use_local_chromedriver(self) -> bool:
    return self.major_version == 0 or self.is_locally_compiled()

  def is_locally_compiled(self) -> bool:
    return pth.LocalPath(self.app_path.parent / "args.gn").exists()

  def _execute_cdp_cmd(self, driver: webdriver.Remote, cmd: str,
                       cmd_args: dict):
    return driver.execute("executeCdpCommand", {
        "cmd": cmd,
        "params": cmd_args
    })["value"]

  def _filter_flags_for_run(self, flags: FlagsT) -> FlagsT:
    assert isinstance(flags, ChromeFlags)
    chrome_flags: ChromeFlags = cast(ChromeFlags, flags)
    for flag in self.UNSUPPORTED_FLAGS:
      if flag not in chrome_flags:
        continue
      flag_value = chrome_flags.pop(flag, None)
      logging.debug("Chromium: Removed unsupported flag: %s=%s", flag,
                    flag_value)
    return chrome_flags  # type: ignore

  def _find_driver(self) -> pth.AnyPath:
    if self._driver_path:
      return self._driver_path
    finder = ChromeDriverFinder(self)
    assert self.app_path
    if self.use_local_chromedriver():
      return finder.find_local_build()
    try:
      return finder.download()
    except DriverNotFoundError as original_download_error:
      logging.debug(
          "Could not download chromedriver, "
          "falling back to finding local build: %s", original_download_error)
      try:
        return finder.find_local_build()
      except DriverNotFoundError as e:
        logging.debug("Could not find fallback chromedriver: %s", e)
        raise original_download_error from e
      # to make an old pytype version happy
      return pth.LocalPath()

  def _start_driver(self, session: BrowserSessionRunGroup,
                    driver_path: pth.AnyPath) -> webdriver.Remote:
    return self._start_chromedriver(session, driver_path)

  def _start_chromedriver(self, session: BrowserSessionRunGroup,
                          driver_path: pth.AnyPath) -> ChromiumDriver:
    assert not self._is_running
    assert self.log_file
    args = self._get_browser_flags_for_session(session)
    options = self._create_options(session, args)

    self._log_browser_start(args, driver_path)
    service_args: List[str] = []
    driver_log_path: Optional[str] = None
    if self._settings.driver_logging:
      service_args += ["--verbose"]
      driver_log_path = os.fspath(self._setup_driver_log_file())
    adb_port = os.environ.get("ANDROID_ADB_SERVER_PORT")
    if adb_port and adb_port.isdigit():
      service_args += ["--adb-port=" + adb_port]
    # pytype: disable=wrong-keyword-args
    service = self.WEB_DRIVER_SERVICE(
        executable_path=os.fspath(driver_path),
        log_output=driver_log_path,  # type: ignore
        # TODO: remove after upgrading the vpython selenium version.
        log_path=driver_log_path,
        service_args=service_args)
    # TODO: support remote platforms
    driver = self._create_driver(options, service)
    # pytype: enable=wrong-keyword-args
    # Prevent debugging overhead.
    self._execute_cdp_cmd(driver, "Runtime.setMaxCallStackSizeToCapture",
                          {"size": 0})
    return driver

  def _create_options(self, session: BrowserSessionRunGroup,
                      args: Sequence[str]) -> ChromiumOptions:
    assert not self._is_running
    options: ChromiumOptions = self.WEB_DRIVER_OPTIONS()
    options.set_capability("browserVersion", str(self.major_version))
    # Don't wait for document-ready.
    options.set_capability("pageLoadStrategy", "eager")
    for arg in args:
      options.add_argument(arg)
    options.binary_location = os.fspath(self.path)
    session.setup_selenium_options(options)
    return options

  @abc.abstractmethod
  def _create_driver(self, options: ChromiumOptions,
                     service: ChromiumService) -> ChromiumDriver:
    pass

  def _validate_driver_version(self) -> None:
    assert self._driver_path, "No driver available"
    error_message = None
    if self.is_local and helper.is_build_dir(
        self.platform.local_path(self.app_path.parent), self.platform):
      error_message = self._validate_locally_built_driver(
          self.platform.local_path(self._driver_path))
    else:
      error_message = self._validate_any_driver_version(self._driver_path)
    if error_message:
      raise RuntimeError("\n".join(error_message))

  def _validate_locally_built_driver(
      self, driver_path: pth.LocalPath) -> Optional[Iterable[str]]:
    # TODO: migrate to version object on the browser
    browser_version = ChromiumVersion.parse(self.version)
    driver_version = ChromeDriverVersion.parse(
        self.platform.app_version(driver_path))
    if browser_version.parts == driver_version.parts:
      return None
    return (f"Chromedriver version mismatch: driver={driver_version.parts_str} "
            f"browser={browser_version.parts_str} ({self}).",
            helper.build_chromedriver_instructions(driver_path.parent))

  def _validate_any_driver_version(
      self, driver_path: pth.AnyPath) -> Optional[Iterable[str]]:
    raw_version_str = self.host_platform.sh_stdout(driver_path, "--version")
    driver_version = ChromeDriverVersion.parse(raw_version_str)
    if driver_version.major == self.major_version:
      return None
    return (f"Chromedriver version mismatch: driver={driver_version} "
            f"browser={self.version} ({self})",)

  def run_script_on_new_document(self, script: str) -> None:
    self._execute_cdp_cmd(self._private_driver,
                          "Page.addScriptToEvaluateOnNewDocument",
                          {"source": script})

  def current_window_id(self) -> str:
    return str(self._private_driver.current_window_handle)

  def switch_window(self, window_id: str) -> None:
    self._private_driver.switch_to.window(window_id)

  def switch_tab(
      self,
      title: Optional[re.Pattern] = None,
      url: Optional[re.Pattern] = None,
      tab_index: Optional[int] = None,
      timeout: dt.timedelta = dt.timedelta(seconds=0)
  ) -> None:
    driver = self._private_driver
    original_handle = driver.current_window_handle
    for _ in wait.wait_with_backoff(timeout):
      # Search through other handles starting from current_window_handle + 1
      try:
        i = driver.window_handles.index(original_handle)
      except ValueError as e:
        raise RuntimeError("Original starting tab no longer exists") from e

      if tab_index is not None:
        handles = [driver.window_handles[tab_index]]
      else:
        handles = driver.window_handles[i + 1:] + driver.window_handles[:i]

      for handle in handles:
        driver.switch_to.window(handle)
        if title is not None:
          if title.match(driver.title) is None:
            continue
        if url is not None:
          if url.match(driver.current_url) is None:
            continue
        return
    error = "No new tab found"
    if title is not None:
      error += f" with title matching {repr(title.pattern)}"
    if url is not None:
      error += f" with url matching {repr(url.pattern)}"
    if tab_index is not None:
      error += f" with tab_index matching {tab_index}"
    raise RuntimeError(error)

  def start_profiling(self) -> None:
    assert isinstance(self._private_driver, ChromiumDriver)
    # TODO: reuse the TraceProbe categories,
    self._execute_cdp_cmd(
        self._private_driver, "Tracing.start", {
            "transferMode":
                "ReturnAsStream",
            "includedCategories": [
                "devtools.timeline",
                "v8.execute",
                "disabled-by-default-devtools.timeline",
                "disabled-by-default-devtools.timeline.frame",
                "toplevel",
                "blink.console",
                "blink.user_timing",
                "latencyInfo",
                "disabled-by-default-devtools.timeline.stack",
                "disabled-by-default-v8.cpu_profiler",
            ],
        })

  def stop_profiling(self) -> Any:
    assert isinstance(self._private_driver, ChromiumDriver)
    data = self._execute_cdp_cmd(self._private_driver,
                                 "Tracing.tracingComplete", {})
    # TODO: use webdriver bidi to get the async Tracing.end event.
    # self._execute_cdp_cmd(self._driver, "Tracing.end", {})
    return data
