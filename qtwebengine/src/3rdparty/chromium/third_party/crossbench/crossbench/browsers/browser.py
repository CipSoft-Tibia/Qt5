# Copyright 2022 The Chromium Authors
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

from __future__ import annotations

import abc
import datetime as dt
import logging
import os
import shlex
from typing import TYPE_CHECKING, Any, Iterable, Optional, Sequence, Tuple

from ordered_set import OrderedSet

from crossbench import path as pth
from crossbench import plt
from crossbench.browsers.settings import Settings
from crossbench.flags.base import Flags, FlagsData, FlagsT

if TYPE_CHECKING:
  import re

  from crossbench.browsers.attributes import BrowserAttributes
  from crossbench.browsers.viewport import Viewport
  from crossbench.cli.config.secrets import Secrets, UsernamePassword
  from crossbench.env import HostEnvironment
  from crossbench.flags.chrome import ChromeFeatures
  from crossbench.flags.js_flags import JSFlags
  from crossbench.network.base import Network
  from crossbench.probes.probe import Probe
  from crossbench.runner.groups.session import BrowserSessionRunGroup
  from crossbench.types import JsonDict


class Browser(abc.ABC):

  @classmethod
  def default_flags(cls, initial_data: FlagsData = None) -> Flags:
    return Flags(initial_data)

  def __init__(self,
               label: str,
               path: Optional[pth.AnyPath] = None,
               settings: Optional[Settings] = None):
    self._settings = settings or Settings()
    self._platform = self._settings.platform
    self.label: str = label
    self._unique_name: str = ""
    self.app_name: str = self.type_name
    self.version: str = "custom"
    self.major_version: int = 0
    self.app_path: pth.AnyPath = pth.AnyPath()
    self.path = pth.AnyPath()
    self._setup_path(path)
    self._is_running: bool = False
    self._pid: Optional[int] = None
    self._probes: OrderedSet[Probe] = OrderedSet()
    self._flags: Flags = self._setup_flags(self._settings)
    self.log_file: Optional[pth.AnyPath] = None
    self.cache_dir: Optional[pth.AnyPath] = self._settings.cache_dir
    self.clear_cache_dir: bool = True
    self._setup_cache_dir(self._settings)

  def _setup_path(self, path: Optional[pth.AnyPath] = None) -> None:
    if not path:
      # TODO: separate class for remote browser (selenium) without an explicit
      # binary path.
      self.unique_name = f"{self.type_name}_{self.label}".lower()
      return
    self.path = self._resolve_binary(path)
    # TODO clean up
    if not self.platform.is_android:
      assert self.path.is_absolute()
    self.version = self._extract_version()
    self.major_version = int(self.version.split(".")[0])
    self.unique_name = f"{self.type_name}_v{self.major_version}_{self.label}"

  def _setup_flags(self, settings: Settings) -> Flags:
    assert not self._settings.js_flags, (
        f"{self} doesn't support custom js_flags")
    return self.default_flags(settings.flags)

  def _setup_cache_dir(self, settings: Settings) -> None:
    pass

  @property
  @abc.abstractmethod
  def type_name(self) -> str:
    pass

  @property
  @abc.abstractmethod
  def attributes(self) -> BrowserAttributes:
    pass

  @property
  def platform(self) -> plt.Platform:
    return self._platform

  @property
  def host_platform(self) -> plt.Platform:
    return self._platform.host_platform

  @property
  def unique_name(self) -> str:
    return self._unique_name

  @unique_name.setter
  def unique_name(self, name: str) -> None:
    assert name
    # Replace any potentially unsafe chars in the name
    self._unique_name = pth.safe_filename(name).lower()

  @property
  def driver_logging(self) -> bool:
    return self._settings.driver_logging

  @property
  def network(self) -> Network:
    return self._settings.network

  @property
  def secrets(self) -> Secrets:
    return self._settings.secrets

  @property
  def settings(self):
    return self._settings

  @property
  def viewport(self) -> Viewport:
    return self._settings.viewport

  @viewport.setter
  def viewport(self, value: Viewport) -> None:
    self._settings.viewport = value

  @property
  def wipe_system_user_data(self) -> bool:
    return self._settings.wipe_system_user_data

  @property
  def http_request_timeout(self) -> dt.timedelta:
    return self._settings.http_request_timeout

  @property
  def driver_path(self) -> Optional[pth.AnyPath]:
    return self._settings.driver_path

  @property
  def probes(self) -> Iterable[Probe]:
    return iter(self._probes)

  @property
  def flags(self) -> Flags:
    return self._flags

  @property
  def features(self) -> ChromeFeatures:
    raise NotImplementedError(f"Unsupported feature flags on {self}.")

  @property
  def js_flags(self) -> JSFlags:
    raise NotImplementedError(f"Unsupported feature flags on {self}.")

  def user_agent(self) -> str:
    return str(self.js("return window.navigator.userAgent"))

  @property
  def pid(self) -> Optional[int]:
    return self._pid

  @property
  def is_running_process(self) -> Optional[bool]:
    # TODO: activate this method again
    if self.pid is None:
      return None
    info = self.platform.process_info(self.pid)
    if info is None:
      return None
    if status := info.get("status"):
      return status in ("running", "sleeping")
    # TODO(cbruni): fix posix process_info for remote platforms where
    # we don't get the status back.
    return False

  @property
  def is_running(self) -> bool:
    return self._is_running

  def validate_env(self, env: HostEnvironment) -> None:
    """Called before starting a browser / browser session to perform
    a pre-run checklist."""

  @property
  def is_local(self) -> bool:
    return self.platform.is_local

  @property
  def is_remote(self) -> bool:
    return self.platform.is_remote

  def set_log_file(self, path: pth.AnyPath) -> None:
    self.log_file = path

  @property
  def stdout_log_file(self) -> pth.AnyPath:
    assert self.log_file
    return self.log_file.with_suffix(".stdout.log")

  @property
  def driver_log_file(self) -> Optional[pth.LocalPath]:
    return None

  def _resolve_binary(self, path: pth.AnyPath) -> pth.AnyPath:
    path = self.platform.absolute(path)
    assert self.platform.exists(path), f"Binary at path={path} does not exist."
    self.app_path = path
    self.app_name = self.app_path.stem
    if self.platform.is_macos:
      path = self._resolve_macos_binary(path)
    assert self.platform.is_file(path), (
        f"Binary at path={path} is not a file.")
    return path

  def _resolve_macos_binary(self, path: pth.AnyPath) -> pth.AnyPath:
    assert self.platform.is_macos
    candidate = self.platform.search_binary(path)
    if not candidate or not self.platform.is_file(candidate):
      raise ValueError(f"Could not find browser executable in {path}")
    return candidate

  def attach_probe(self, probe: Probe) -> None:
    if probe in self._probes:
      raise ValueError(f"Cannot attach same probe twice: {probe}")
    self._probes.add(probe)
    probe.attach(self)

  def details_json(self) -> JsonDict:
    return {
        "label": self.label,
        "browser": self.type_name,
        "unique_name": self.unique_name,
        "app_name": self.app_name,
        "version": self.version,
        "flags": tuple(self.flags),
        "js_flags": tuple(),
        "path": os.fspath(self.path),
        "clear_cache_dir": self.clear_cache_dir,
        "major_version": self.major_version,
        "log": {}
    }

  def validate_flags(self) -> None:
    """ Helper method is called from the Runner before any Runs / Sessions
    have started."""

  def validate_binary(self) -> None:
    """ Helper method is called from the Runner before any Runs / Sessions
    have started."""

  def setup_binary(self) -> None:
    """ This helper is called in the setup steps of each Session.
    This can be used to install a custom binary on remote devices. """

  def setup(self, session: BrowserSessionRunGroup) -> None:
    assert not self._is_running, (
        "Previously used browser was not correctly stopped.")
    self.clear_cache()
    self.start(session)
    assert self._is_running

  def is_logged_in(self,
                   secret: UsernamePassword,
                   strict: bool = False) -> bool:
    """Determines whether the browser is already logged in with the given
    credentials.

    Args:
      secret: The credentials to check.
      strict: Whether or not to raise an error if login is impossible

    Returns:
      True if and only if the browser is already logged in with the account

    Raises:
      RuntimeError: If strict, when logging in with the given cridentials is
      not possible.
    """
    del secret
    del strict
    return False

  @abc.abstractmethod
  def _extract_version(self) -> str:
    pass

  def clear_cache(self) -> None:
    if self.clear_cache_dir and self.cache_dir:
      self.platform.rm(self.cache_dir, missing_ok=True, dir=True)
      self.platform.mkdir(self.cache_dir, parents=True)

  @abc.abstractmethod
  def start(self, session: BrowserSessionRunGroup) -> None:
    pass

  def _log_browser_start(self,
                         args: Tuple[str, ...],
                         driver_path: Optional[pth.AnyPath] = None) -> None:
    logging.info("STARTING BROWSER Binary:  %s", self.path)
    logging.info("STARTING BROWSER Version: %s", self.version)
    if driver_path:
      logging.info("STARTING BROWSER Driver:  %s", driver_path)
    logging.info("STARTING BROWSER Network: %s", self.network)
    logging.info("STARTING BROWSER Probes:  %s",
                 ", ".join(p.NAME for p in self.probes))
    logging.info("STARTING BROWSER Flags:   %s", shlex.join(args))

  def _get_browser_flags_for_session(
      self, session: BrowserSessionRunGroup) -> Tuple[str, ...]:
    flags_copy: Flags = self.flags.copy()
    flags_copy.update(session.extra_flags)
    flags_copy.update(self.network.extra_flags(self.attributes))
    flags_copy = self._filter_flags_for_run(flags_copy)
    return tuple(flags_copy)

  def _filter_flags_for_run(self, flags: FlagsT) -> FlagsT:
    return flags

  def quit(self) -> None:
    assert self._is_running, "Browser is already stopped"
    try:
      self.force_quit()
    finally:
      self._pid = None

  def force_quit(self) -> None:
    if not self._is_running:
      return
    logging.info("Browser.force_quit()")
    if self.platform.is_macos:
      self.platform.exec_apple_script(f"""
  tell application "{self.app_path}"
    quit
  end tell
      """)
    elif self._pid:
      self.platform.terminate(self._pid)
    self._is_running = False

  @abc.abstractmethod
  def js(
      self,
      script: str,
      timeout: Optional[dt.timedelta] = None,
      arguments: Sequence[object] = ()
  ) -> Any:
    pass

  def run_script_on_new_document(self, script: str) -> None:
    del script
    raise NotImplementedError(
        f"New document script injection is not supported by {self}")

  def current_window_id(self) -> str:
    raise NotImplementedError(f"current_window_id is not implemented by {self}")

  def switch_window(self, window_id: str) -> None:
    del window_id
    raise NotImplementedError(f"switch_window is not implemented by {self}")

  def switch_tab(
      self,
      title: Optional[re.Pattern] = None,
      url: Optional[re.Pattern] = None,
      tab_index: Optional[int] = None,
      timeout: dt.timedelta = dt.timedelta(seconds=0)
  ) -> None:
    del title
    del url
    del tab_index
    del timeout
    raise NotImplementedError(f"Switching tabs is not supported by {self}")

  @abc.abstractmethod
  def show_url(self, url: str, target: Optional[str] = None) -> None:
    pass

  def switch_to_new_tab(self) -> None:
    raise NotImplementedError(f"New tab is not supported by {self}")

  def screenshot(self, path: pth.LocalPath) -> None:
    # TODO: implement screenshot on browser and platform.
    raise NotImplementedError(f"Taking screenshots is not supported by {self}")

  def _sync_viewport_flag(self, flags: Flags, flag: str,
                          is_requested_by_viewport: bool,
                          replacement: Viewport) -> None:
    if is_requested_by_viewport:
      flags.set(flag)
    elif flag in flags:
      if self.viewport.is_default:
        self.viewport = replacement
      else:
        raise ValueError(
            f"{flag} conflicts with requested --viewport={self.viewport}")

  def __str__(self) -> str:
    platform_prefix = ""
    if self.platform.is_remote:
      platform_prefix = str(self.platform)
    return f"{platform_prefix}{self.type_name.capitalize()}:{self.label}"

  def __hash__(self) -> int:
    # Poor-man's hash, browsers should be unique.
    return hash(id(self))

  def performance_mark(self, name: str):
    self.js("performance.mark(arguments[0]);", arguments=[name])
