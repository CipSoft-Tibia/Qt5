# Copyright 2023 The Chromium Authors
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

from __future__ import annotations

import abc
import atexit
import json
import logging
import os
import re
import shutil
import stat
import tempfile
import urllib.error
import zipfile
from typing import (TYPE_CHECKING, Any, Dict, Final, Iterable, List, Optional,
                    Sequence, Tuple, Type, cast)

from selenium.webdriver.chromium.options import ChromiumOptions
from selenium.webdriver.chromium.service import ChromiumService
from selenium.webdriver.chromium.webdriver import ChromiumDriver
from selenium.webdriver.remote.webdriver import WebDriver as RemoteWebDriver

from crossbench import exception, helper
from crossbench import path as pth
from crossbench.browsers.attributes import BrowserAttributes
from crossbench.browsers.browser_helper import BROWSERS_CACHE
from crossbench.browsers.chromium.chromium import Chromium
from crossbench.browsers.chromium.version import (ChromeDriverVersion,
                                                  ChromiumVersion)
from crossbench.browsers.webdriver import WebDriverBrowser
from crossbench.flags.chrome import ChromeFlags
from crossbench.plt.android_adb import AndroidAdbPlatform
from crossbench.plt.chromeos_ssh import ChromeOsSshPlatform
from crossbench.plt.linux_ssh import LinuxSshPlatform

if TYPE_CHECKING:
  from selenium import webdriver

  from crossbench.flags.base import FlagsT
  from crossbench.plt.base import Platform
  from crossbench.runner.groups import BrowserSessionRunGroup
  from crossbench.runner.runner import Runner


class ChromiumWebDriver(WebDriverBrowser, Chromium, metaclass=abc.ABCMeta):

  WEB_DRIVER_OPTIONS: Type[ChromiumOptions] = ChromiumOptions
  WEB_DRIVER_SERVICE: Type[ChromiumService] = ChromiumService

  @property
  def attributes(self) -> BrowserAttributes:
    return (BrowserAttributes.CHROMIUM | BrowserAttributes.CHROMIUM_BASED
            | BrowserAttributes.WEBDRIVER)

  def use_local_chromedriver(self) -> bool:
    return self.major_version == 0 or self.is_locally_compiled()

  def is_locally_compiled(self) -> bool:
    return pth.LocalPath(self.app_path.parent / "args.gn").exists()

  def _find_driver(self) -> pth.RemotePath:
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
                    driver_path: pth.RemotePath) -> webdriver.Remote:
    return self._start_chromedriver(session, driver_path)

  def _start_chromedriver(self, session: BrowserSessionRunGroup,
                          driver_path: pth.RemotePath) -> ChromiumDriver:
    assert not self._is_running
    assert self.log_file
    args = self._get_browser_flags_for_session(session)
    options = self._create_options(session, args)

    self._log_browser_start(args, driver_path)

    # pytype: disable=wrong-keyword-args
    service = self.WEB_DRIVER_SERVICE(
        executable_path=str(driver_path),
        log_path=str(self.driver_log_file),
        # TODO: support clean logging of chrome stdout / stderr
        service_args=["--verbose"])
    # TODO: support remote platforms
    service.log_file = pth.LocalPath(self.stdout_log_file).open(  # pylint: disable=consider-using-with
        "w", encoding="utf-8")
    driver: ChromiumDriver = self._create_driver(options, service)
    # pytype: enable=wrong-keyword-args
    # Prevent debugging overhead.
    driver.execute_cdp_cmd("Runtime.setMaxCallStackSizeToCapture", {"size": 0})
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
    options.binary_location = str(self.path)
    session.setup_selenium_options(options)
    return options

  @abc.abstractmethod
  def _create_driver(self, options: ChromiumOptions,
                     service: ChromiumService) -> ChromiumDriver:
    pass

  def _validate_driver_version(self) -> None:
    assert self._driver_path, "No driver available"
    error_message = None
    if self.is_local and is_build_dir(
        self.platform.local_path(self.app_path.parent)):
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
            build_chromedriver_instructions(driver_path.parent))

  def _validate_any_driver_version(
      self, driver_path: pth.RemotePath) -> Optional[Iterable[str]]:
    raw_version_str = self.platform.host_platform.sh_stdout(
        driver_path, "--version")
    driver_version = ChromeDriverVersion.parse(raw_version_str)
    if driver_version.major == self.major_version:
      return None
    return (f"Chromedriver version mismatch: driver={driver_version} "
            f"browser={self.version} ({self})",)

  def run_script_on_new_document(self, script: str) -> None:
    self._driver.execute_cdp_cmd("Page.addScriptToEvaluateOnNewDocument",
                                 {"source": script})

  def start_profiling(self) -> None:
    assert isinstance(self._driver, ChromiumDriver)
    # TODO: reuse the TraceProbe categories,
    self._driver.execute_cdp_cmd(
        "Tracing.start", {
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
    assert isinstance(self._driver, ChromiumDriver)
    data = self._driver.execute_cdp_cmd("Tracing.tracingComplete", {})
    # TODO: use webdriver bidi to get the async Tracing.end event.
    # self._driver.execute_cdp_cmd("Tracing.end", {})
    return data


# Android is high-tech and reads chrome flags from an app-specific file.
# TODO: extend support to more than just chrome.
_FLAG_ROOT: pth.RemotePath = pth.RemotePath("/data/local/tmp/")
FLAGS_WEBLAYER: pth.RemotePath = _FLAG_ROOT / "weblayer-command-line"
FLAGS_WEBVIEW: pth.RemotePath = _FLAG_ROOT / "webview-command-line"
FLAGS_CONTENT_SHELL: pth.RemotePath = _FLAG_ROOT / "content-shell-command-line"
FLAGS_CHROME: pth.RemotePath = _FLAG_ROOT / "chrome-command-line"

class ChromiumWebDriverAndroid(ChromiumWebDriver):

  def __init__(self, *args, **kwargs):
    self._chrome_command_line_path: pth.RemotePath = FLAGS_CHROME
    self._previous_command_line_contents: Optional[str] = None
    super().__init__(*args, **kwargs)
    self._android_package: str = self.platform.app_path_to_package(self.path)
    if not self._android_package:
      raise RuntimeError("Could not find matching adb package for "
                         f"{self.path} on {self.platform}")

  @property
  def android_package(self) -> str:
    return self._android_package

  @property
  def platform(self) -> AndroidAdbPlatform:
    assert isinstance(
        self._platform,
        AndroidAdbPlatform), (f"Invalid platform: {self._platform}")
    return cast(AndroidAdbPlatform, self._platform)

  def _resolve_binary(self, path: pth.RemotePath) -> pth.RemotePath:
    return path

  # TODO: implement setting a clean profile on android
  _UNSUPPORTED_FLAGS: Tuple[str, ...] = (
      "--user-data-dir",
      "--disable-sync",
      "--window-size",
      "--window-position",
  )

  def _filter_flags_for_run(self, flags: FlagsT) -> FlagsT:
    assert isinstance(flags, ChromeFlags)
    chrome_flags = cast(ChromeFlags, flags)
    for flag in self._UNSUPPORTED_FLAGS:
      if flag not in chrome_flags:
        continue
      flag_value = chrome_flags.pop(flag, None)
      logging.debug("Chrome Android: Removed unsupported flag: %s=%s", flag,
                    flag_value)
    return chrome_flags

  def _start_driver(self, session: BrowserSessionRunGroup,
                    driver_path: pth.RemotePath) -> webdriver.Remote:
    self.adb_force_stop()
    self._backup_chrome_flags()
    atexit.register(self._restore_chrome_flags)
    return self._start_chromedriver(session, driver_path)

  def _backup_chrome_flags(self) -> None:
    assert self._previous_command_line_contents is None
    self._previous_command_line_contents = self._read_device_flags()

  def _read_device_flags(self) -> Optional[str]:
    if not self.platform.exists(self._chrome_command_line_path):
      return None
    return self.platform.cat(self._chrome_command_line_path)

  def adb_force_stop(self) -> None:
    self.platform.adb.force_stop(self.android_package)

  def force_quit(self) -> None:
    try:
      try:
        super().force_quit()
      finally:
        self.adb_force_stop()
    finally:
      self._restore_chrome_flags()

  def _restore_chrome_flags(self) -> None:
    atexit.unregister(self._restore_chrome_flags)
    if self._previous_command_line_contents is None:
      return
    current_flags = self._read_device_flags()
    if current_flags != self._previous_command_line_contents:
      logging.warning("%s: flags file changed during run", self)
    if self._previous_command_line_contents is None:
      logging.debug("%s: deleting chrome flags file: %s", self,
                    self._chrome_command_line_path)
      self.platform.rm(self._chrome_command_line_path, missing_ok=True)
    else:
      logging.debug("%s: restoring previous flags file contents in %s", self,
                    self._chrome_command_line_path)
      self.platform.set_file_contents(self._chrome_command_line_path,
                                      self._previous_command_line_contents)
      self._previous_command_line_contents = None

  def _create_options(self, session: BrowserSessionRunGroup,
                      args: Sequence[str]) -> ChromiumOptions:
    options: ChromiumOptions = super()._create_options(session, args)
    options.binary_location = ""
    options.add_experimental_option("androidPackage", self.android_package)
    options.add_experimental_option("androidDeviceSerial",
                                    self.platform.adb.serial_id)
    return options

  def setup_binary(self, runner: Runner) -> None:
    super().setup_binary(runner)
    self.platform.adb.grant_notification_permissions(self.android_package)


class ChromiumWebDriverSsh(ChromiumWebDriver):

  @property
  def platform(self) -> LinuxSshPlatform:
    assert isinstance(self._platform,
                      LinuxSshPlatform), (f"Invalid platform: {self._platform}")
    return cast(LinuxSshPlatform, self._platform)

  def _start_driver(self, session: BrowserSessionRunGroup,
                    driver_path: pth.RemotePath) -> RemoteWebDriver:
    del driver_path
    args = self._get_browser_flags_for_session(session)
    options = self._create_options(session, args)
    platform = self.platform
    host = platform.host
    port = platform.port
    driver = RemoteWebDriver(f"http://{host}:{port}", options=options)
    return driver


class ChromiumWebDriverChromeOsSsh(ChromiumWebDriver):

  @property
  def platform(self) -> ChromeOsSshPlatform:
    assert isinstance(
        self._platform,
        ChromeOsSshPlatform), (f"Invalid platform: {self._platform}")
    return cast(ChromeOsSshPlatform, self._platform)

  def _start_driver(self, session: BrowserSessionRunGroup,
                    driver_path: pth.RemotePath) -> RemoteWebDriver:
    del driver_path
    platform = self.platform
    host = platform.host
    port = platform.port
    args = self._get_browser_flags_for_session(session)
    # TODO(spadhi): correctly handle flags:
    #   1. decide which flags to pass to chrome vs chromedriver
    #   2. investigate irrelevant / unsupported flags on ChromeOS
    #   3. filter out and pass the chrome flags to the debugging session below
    #   4. pass the remaining flags to RemoteWebDriver options
    dbg_port = platform.create_debugging_session()
    options = self._create_options(session, args)
    options.add_experimental_option("debuggerAddress", f"127.0.0.1:{dbg_port}")
    driver = RemoteWebDriver(f"http://{host}:{port}", options=options)
    return driver


class DriverNotFoundError(ValueError):
  pass

def build_chromedriver_instructions(build_dir: pth.RemotePath) -> str:
  return ("Please build 'chromedriver' manually for local builds:\n"
          f"    autoninja -C {build_dir} chromedriver")


def is_build_dir(path: pth.LocalPath) -> bool:
  return (path / "args.gn").is_file()

class ChromeDriverFinder:
  driver_path: pth.LocalPath

  def __init__(self,
               browser: ChromiumWebDriver,
               cache_dir: pth.LocalPath = BROWSERS_CACHE):
    self.browser = browser
    self.platform: Platform = browser.platform
    self.host_platform: Platform = browser.platform.host_platform
    extension: str = ""
    if self.host_platform.is_win:
      extension = ".exe"
    self.driver_path: pth.LocalPath = (
        cache_dir / f"chromedriver-{self.browser.major_version}{extension}")
    self._validate_browser()

  def _validate_browser(self) -> None:
    browser_platform = self.browser.platform
    if browser_platform.is_local:
      return
    # Some remote platforms rely on a local chromedriver
    if (browser_platform.is_android or browser_platform.is_remote_ssh):
      return
    raise RuntimeError("Cannot download chromedriver for remote browser yet")

  def find_local_build(self) -> pth.LocalPath:
    assert self.browser.app_path
    # assume it's a local build
    lookup_dir = pth.LocalPath(self.browser.app_path.parent)
    driver_path = lookup_dir / "chromedriver"
    if driver_path.is_file():
      return driver_path
    error_message: List[str] = [f"Driver '{driver_path}' does not exist."]
    if is_build_dir(lookup_dir):
      error_message += [build_chromedriver_instructions(lookup_dir)]
    else:
      error_message += ["Please manually provide a chromedriver binary."]
    raise DriverNotFoundError("\n".join(error_message))

  def download(self) -> pth.LocalPath:
    if not self.driver_path.is_file():
      with exception.annotate(
          f"Downloading chromedriver for {self.browser.version}"):
        self._download()
    return self.driver_path

  def _download(self) -> None:
    milestone = self.browser.major_version
    logging.info("CHROMEDRIVER Downloading from %s v%s", self.browser.type_name,
                 milestone)
    url: Optional[str] = None
    listing_url: Optional[str] = None
    if milestone >= self.CFT_MIN_MILESTONE:
      listing_url, url = self._get_cft_url(milestone)
    if not url:
      listing_url, url = self._get_pre_115_stable_url(milestone)
      if not url:
        listing_url, url = self._get_canary_url()

    if not url:
      raise DriverNotFoundError(
          "Please manually compile/download chromedriver for "
          f"{self.browser.type_name} {self.browser.version}")

    logging.info("CHROMEDRIVER Downloading M%s: %s", milestone, listing_url or
                 url)
    with tempfile.TemporaryDirectory() as tmp_dir:
      if ".zip" not in url:
        maybe_driver = pth.LocalPath(tmp_dir) / "chromedriver"
        self.host_platform.download_to(url, maybe_driver)
      else:
        zip_file = pth.LocalPath(tmp_dir) / "download.zip"
        self.host_platform.download_to(url, zip_file)
        with zipfile.ZipFile(zip_file, "r") as zip_ref:
          zip_ref.extractall(zip_file.parent)
        zip_file.unlink()
        maybe_driver = None
        candidates: List[pth.LocalPath] = [
            path for path in zip_file.parent.glob("**/*")
            if path.is_file() and "chromedriver" in path.name
        ]
        # Find exact match first:
        maybe_drivers: List[pth.LocalPath] = [
            path for path in candidates if path.stem == "chromedriver"
        ]
        # Backup less strict matching:
        maybe_drivers += candidates
        if len(maybe_drivers) > 0:
          maybe_driver = maybe_drivers[0]
      if not maybe_driver or not maybe_driver.is_file():
        raise DriverNotFoundError(
            f"Extracted driver at {maybe_driver} does not exist.")
      self.driver_path.parent.mkdir(parents=True, exist_ok=True)
      shutil.move(os.fspath(maybe_driver), self.driver_path)
      self.driver_path.chmod(self.driver_path.stat().st_mode | stat.S_IEXEC)

  # Using CFT as abbreviation for Chrome For Testing here.
  CFT_MIN_MILESTONE = 115
  CFT_BASE_URL: str = "https://googlechromelabs.github.io/chrome-for-testing"
  CFT_VERSION_URL: str = f"{CFT_BASE_URL}/{{version}}.json"
  CFT_LATEST_URL: str = f"{CFT_BASE_URL}/LATEST_RELEASE_{{major}}"

  CFT_PLATFORM: Final[Dict[Tuple[str, str], str]] = {
      ("linux", "x64"): "linux64",
      ("macos", "x64"): "mac-x64",
      ("macos", "arm64"): "mac-arm64",
      ("win", "ia32"): "win32",
      ("win", "x64"): "win64"
  }

  def _get_cft_url(self, milestone: int) -> Tuple[str, Optional[str]]:
    logging.debug("ChromeDriverFinder: Looking up chrome-for-testing version.")
    platform_name: Optional[str] = self.CFT_PLATFORM.get(self.host_platform.key)
    if not platform_name:
      raise DriverNotFoundError(
          f"Unsupported platform {self.host_platform.key} for chromedriver.")
    listing_url, version_data = self._get_cft_version_data(milestone)
    download_url: Optional[str] = None
    if version_data:
      download_url = self._get_cft_driver_download_url(version_data,
                                                       platform_name)
    return (listing_url, download_url)

  def _get_cft_version_data(self, milestone: int) -> Tuple[str, Optional[Dict]]:
    logging.debug("ChromeDriverFinder: Trying direct download url")
    listing_url, data = self._get_cft_precise_version_data(self.browser.version)
    if data:
      return listing_url, data
    logging.debug(
        "ChromeDriverFinder: Invalid precise version url %s, "
        "using M%s", listing_url, milestone)
    return self._get_ctf_milestone_data(milestone)

  def _get_cft_precise_version_data(self,
                                    version: str) -> Tuple[str, Optional[Dict]]:
    version_url = self.CFT_VERSION_URL.format(version=version)
    try:
      with helper.urlopen(version_url) as response:
        version_data = json.loads(response.read().decode("utf-8"))
        return (version_url, version_data)
    except urllib.error.HTTPError as e:
      logging.debug("ChromeDriverFinder: "
                    "Precise version download failed %s", e)
      return (version_url, None)

  def _get_ctf_milestone_data(self,
                              milestone: int) -> Tuple[str, Optional[Dict]]:
    latest_version_url = self.CFT_LATEST_URL.format(major=milestone)
    try:
      with helper.urlopen(latest_version_url) as response:
        alternative_version = response.read().decode("utf-8").strip()
        logging.debug(
            "ChromeDriverFinder: Using alternative version %s "
            "for M%s", alternative_version, milestone)
        return self._get_cft_precise_version_data(alternative_version)
    except urllib.error.HTTPError:
      return (self.CFT_BASE_URL, None)

  def _get_cft_driver_download_url(self, version_data,
                                   platform_name) -> Optional[str]:
    if all_downloads := version_data.get("downloads"):
      driver_downloads: Dict = all_downloads.get("chromedriver", [])
      for download in driver_downloads:
        if isinstance(download, dict) and download["platform"] == platform_name:
          return download["url"]
    return None

  PRE_115_STABLE_URL: str = "http://chromedriver.storage.googleapis.com"

  def _get_pre_115_stable_url(self,
                              milestone: int) -> Tuple[str, Optional[str]]:
    logging.debug(
        "ChromeDriverFinder: "
        "Looking upe old-style stable version M%s", milestone)
    assert milestone < self.CFT_MIN_MILESTONE
    listing_url = f"{self.PRE_115_STABLE_URL}/index.html"
    driver_version: Optional[str] = self._get_pre_115_driver_version(milestone)
    if not driver_version:
      return listing_url, None
    if self.host_platform.is_linux:
      arch_suffix = "linux64"
    elif self.host_platform.is_macos:
      arch_suffix = "mac64"
      if self.host_platform.is_arm64:
        # The uploaded chromedriver archives changed the naming scheme after
        # chrome version 106.0.5249.21 for Arm64 (previously m1):
        #   before: chromedriver_mac64_m1.zip
        #   after:  chromedriver_mac_arm64.zip
        last_old_naming_version = (106, 0, 5249, 21)
        version_tuple = tuple(map(int, driver_version.split(".")))
        if version_tuple <= last_old_naming_version:
          arch_suffix = "mac64_m1"
        else:
          arch_suffix = "mac_arm64"
    elif self.host_platform.is_win:
      arch_suffix = "win32"
    else:
      raise DriverNotFoundError("Unsupported chromedriver platform")
    url = (f"{self.PRE_115_STABLE_URL}/{driver_version}/"
           f"chromedriver_{arch_suffix}.zip")
    return listing_url, url

  def _get_pre_115_driver_version(self, milestone) -> Optional[str]:
    if milestone < 70:
      return self._get_pre_70_driver_version(milestone)
    url = f"{self.PRE_115_STABLE_URL}/LATEST_RELEASE_{milestone}"
    try:
      with helper.urlopen(url) as response:
        return response.read().decode("utf-8")
    except urllib.error.HTTPError as e:
      if e.code != 404:
        raise DriverNotFoundError(f"Could not query {url}") from e
      logging.debug("ChromeDriverFinder: Could not load latest release url %s",
                    e)
    return None

  def _get_pre_70_driver_version(self, milestone) -> Optional[str]:
    with helper.urlopen(
        f"{self.PRE_115_STABLE_URL}/2.46/notes.txt") as response:
      lines = response.read().decode("utf-8").splitlines()
    for i, line in enumerate(lines):
      if not line.startswith("---"):
        continue
      [min_version, max_version] = map(int, re.findall(r"\d+", lines[i + 1]))
      if min_version <= milestone <= max_version:
        match = re.search(r"\d\.\d+", line)
        if not match:
          raise DriverNotFoundError(f"Could not parse version number: {line}")
        return match.group(0)
    return None

  CHROMIUM_DASH_URL: str = "https://chromiumdash.appspot.com/fetch_releases"
  CHROMIUM_LISTING_URL: str = (
      "https://www.googleapis.com/storage/v1/b/chromium-browser-snapshots/o/")
  CHROMIUM_DASH_PARAMS: Dict[Tuple[str, str], Dict] = {
      ("linux", "x64"): {
          "dash_platform": "linux",
          "dash_channel": "dev",
          "dash_limit": 10,
      },
      ("macos", "x64"): {
          "dash_platform": "mac",
      },
      ("macos", "arm64"): {
          "dash_platform": "mac",
      },
      ("win", "ia32"): {
          "dash_platform": "win",
      },
      ("win", "x64"): {
          "dash_platform": "win64",
      },
  }
  CHROMIUM_LISTING_PREFIX: Dict[Tuple[str, str], str] = {
      ("linux", "x64"): "Linux_x64",
      ("macos", "x64"): "Mac",
      ("macos", "arm64"): "Mac_Arm",
      ("win", "ia32"): "Win",
      ("win", "x64"): "Win_x64",
  }

  def _get_canary_url(self) -> Tuple[str, Optional[str]]:
    logging.debug(
        "ChromeDriverFinder: Try downloading the chromedriver canary version")
    properties = self.CHROMIUM_DASH_PARAMS.get(self.host_platform.key)
    if not properties:
      raise DriverNotFoundError(
          f"Unsupported platform={self.platform}, key={self.host_platform.key}")
    dash_platform = properties["dash_platform"]
    dash_channel = properties.get("dash_channel", "canary")
    # Limit should be > len(canary_versions) so we also get potentially
    # the latest dev version (only beta / stable have official driver binaries).
    dash_limit = properties.get("dash_limit", 100)
    url = helper.update_url_query(
        self.CHROMIUM_DASH_URL, {
            "platform": dash_platform,
            "channel": dash_channel,
            "milestone": str(self.browser.major_version),
            "num": str(dash_limit),
        })
    chromium_base_position = 0
    with helper.urlopen(url) as response:
      version_infos = list(json.loads(response.read().decode("utf-8")))
      if not version_infos:
        raise DriverNotFoundError("Could not find latest version info for "
                                  f"platform={self.host_platform}")
      for version_info in version_infos:
        if version_info["version"] == self.browser.version:
          chromium_base_position = int(
              version_info["chromium_main_branch_position"])
          break

    if not chromium_base_position and version_infos:
      fallback_version_info = None
      # Try matching latest milestone
      for version_info in version_infos:
        if version_info["milestone"] == self.browser.major_version:
          fallback_version_info = version_info
          break

      if not fallback_version_info:
        # Android has a slightly different release cycle than the desktop
        # versions. Assume that the latest canary version is good enough
        fallback_version_info = version_infos[0]
      chromium_base_position = int(
          fallback_version_info["chromium_main_branch_position"])
      logging.warning(
          "Falling back to latest (not precisely matching) "
          "canary chromedriver %s (expected %s)",
          fallback_version_info["version"], self.browser.version)

    if not chromium_base_position:
      raise DriverNotFoundError("Could not find matching canary chromedriver "
                                f"for {self.browser.version}")
    # Use prefixes to limit listing results and increase chances of finding
    # a matching version
    listing_prefix = self.CHROMIUM_LISTING_PREFIX.get(self.host_platform.key)
    if not listing_prefix:
      raise NotImplementedError(
          f"Unsupported chromedriver platform {self.host_platform}")
    base_prefix = str(chromium_base_position)[:4]
    listing_url = helper.update_url_query(self.CHROMIUM_LISTING_URL, {
        "prefix": f"{listing_prefix}/{base_prefix}",
        "maxResults": "10000"
    })
    with helper.urlopen(listing_url) as response:
      listing = json.loads(response.read().decode("utf-8"))

    versions = []
    logging.debug("Filtering %s candidate URLs.", len(listing["items"]))
    for version in listing["items"]:
      if "name" not in version:
        continue
      if "mediaLink" not in version:
        continue
      name = version["name"]
      if "chromedriver" not in name:
        continue
      parts = name.split("/")
      if "chromedriver" not in parts[-1] or len(parts) < 3:
        continue
      base = parts[1]
      try:
        int(base)
      except ValueError:
        # Ignore base if it is not an int
        continue
      versions.append((int(base), version["mediaLink"]))
    versions.sort()
    logging.debug("Found candidates: %s", versions)
    logging.debug("chromium_base_position=%s", chromium_base_position)

    for i in range(len(versions)):
      base, url = versions[i]
      if base > chromium_base_position:
        base, url = versions[i - 1]
        return listing_url, url
    return listing_url, None
