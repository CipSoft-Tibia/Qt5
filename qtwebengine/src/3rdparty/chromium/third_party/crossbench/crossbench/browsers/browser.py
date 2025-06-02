# Copyright 2022 The Chromium Authors
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

from __future__ import annotations

import abc
import logging
import pathlib
import shutil
from typing import TYPE_CHECKING, Any, Iterable, Optional, Sequence, Tuple

from ordered_set import OrderedSet

from crossbench import plt
from crossbench.flags import Flags
from crossbench.network.base import Network
from crossbench.network.live import LiveNetwork

from .splash_screen import SplashScreen
from .viewport import Viewport

if TYPE_CHECKING:
  import datetime as dt

  from crossbench.probes.probe import Probe
  from crossbench.runner.run import Run
  from crossbench.runner.runner import Runner
  from crossbench.types import JsonDict
  from crossbench.runner.groups import BrowserSessionRunGroup


class Browser(abc.ABC):

  @classmethod
  def default_flags(cls, initial_data: Flags.InitialDataType = None) -> Flags:
    return Flags(initial_data)

  def __init__(
      self,
      label: str,
      path: Optional[pathlib.Path] = None,
      flags: Optional[Flags.InitialDataType] = None,
      js_flags: Optional[Flags.InitialDataType] = None,
      cache_dir: Optional[pathlib.Path] = None,
      type: Optional[str] = None,  # pylint: disable=redefined-builtin
      network: Optional[Network] = None,
      driver_path: Optional[pathlib.Path] = None,
      viewport: Optional[Viewport] = None,
      splash_screen: Optional[SplashScreen] = None,
      platform: Optional[plt.Platform] = None):
    self._platform = platform or plt.PLATFORM
    # Marked optional to make subclass constructor calls easier with pytype.
    assert type
    assert not driver_path, "driver_path not supported by base Browser"
    self.type: str = type
    self.label: str = label
    self._unique_name: str = ""
    self.app_name: str = type
    self.version: str = "custom"
    self.major_version: int = 0
    self.app_path: pathlib.Path = pathlib.Path()
    if path:
      self.path = self._resolve_binary(path)
      # TODO clean up
      if not self.platform.is_android:
        assert self.path.is_absolute()
      self.version = self._extract_version()
      self.major_version = int(self.version.split(".")[0])
      self.unique_name = f"{self.type}_v{self.major_version}_{self.label}"
    else:
      # TODO: separate class for remote browser (selenium) without an explicit
      # binary path.
      self.path = pathlib.Path()
      self.unique_name = f"{self.type}_{self.label}".lower()
    self._network: Network = network or LiveNetwork()
    self._viewport: Viewport = viewport or Viewport.DEFAULT
    self._splash_screen: SplashScreen = splash_screen or SplashScreen.DEFAULT
    self._is_running: bool = False
    self.cache_dir: Optional[pathlib.Path] = cache_dir
    self.clear_cache_dir: bool = True
    self._pid: Optional[int] = None
    self._probes: OrderedSet[Probe] = OrderedSet()
    self._flags: Flags = self.default_flags(flags)
    assert not js_flags, "Base Browser doesn't support js_flags directly"
    self.log_file: Optional[pathlib.Path] = None

  @property
  def platform(self) -> plt.Platform:
    return self._platform

  @property
  def unique_name(self) -> str:
    return self._unique_name

  @unique_name.setter
  def unique_name(self, name: str) -> None:
    assert name
    # Replace any potentially unsafe chars in the name
    self._unique_name = plt.safe_filename(name).lower()

  @property
  def network(self) -> Network:
    return self._network

  @property
  def splash_screen(self) -> SplashScreen:
    return self._splash_screen

  @property
  def viewport(self) -> Viewport:
    return self._viewport

  @viewport.setter
  def viewport(self, value: Viewport) -> None:
    assert self._viewport.is_default
    self._viewport = value

  @property
  def probes(self) -> Iterable[Probe]:
    return iter(self._probes)

  @property
  def flags(self) -> Flags:
    return self._flags

  def user_agent(self, runner: Runner) -> str:
    return str(self.js(runner, "return window.navigator.userAgent"))

  @property
  def pid(self) -> Optional[int]:
    return self._pid

  @property
  def is_running(self) -> Optional[bool]:
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
  def is_local(self) -> bool:
    return True

  def set_log_file(self, path: pathlib.Path) -> None:
    self.log_file = path

  @property
  def stdout_log_file(self) -> pathlib.Path:
    assert self.log_file
    return self.log_file.with_suffix(".stdout.log")

  def _resolve_binary(self, path: pathlib.Path) -> pathlib.Path:
    path = path.absolute()
    assert path.exists(), f"Binary at path={path} does not exist."
    self.app_path = path
    self.app_name = self.app_path.stem
    if self.platform.is_macos:
      path = self._resolve_macos_binary(path)
    assert path.is_file(), (f"Binary at path={path} is not a file.")
    return path

  def _resolve_macos_binary(self, path: pathlib.Path) -> pathlib.Path:
    assert self.platform.is_macos
    candidate = self.platform.search_binary(path)
    if not candidate or not candidate.is_file():
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
        "browser": self.type,
        "unique_name": self.unique_name,
        "app_name": self.app_name,
        "version": self.version,
        "flags": tuple(self.flags.get_list()),
        "js_flags": [],
        "path": str(self.path),
        "clear_cache_dir": self.clear_cache_dir,
        "major_version": self.major_version,
        "log": {}
    }

  def setup_binary(self, runner: Runner) -> None:
    pass

  def setup(self, session: BrowserSessionRunGroup) -> None:
    assert not self._is_running
    runner = session.runner
    self.clear_cache(runner)
    self.start(session)
    assert self._is_running

  @abc.abstractmethod
  def _extract_version(self) -> str:
    pass

  def clear_cache(self, runner: Runner) -> None:
    del runner
    if self.clear_cache_dir and self.cache_dir and self.cache_dir.exists():
      shutil.rmtree(self.cache_dir)

  @abc.abstractmethod
  def start(self, session: BrowserSessionRunGroup) -> None:
    pass

  def _get_browser_flags_for_session(
      self, session: BrowserSessionRunGroup) -> Tuple[str, ...]:
    flags_copy: Flags = self.flags.copy()
    flags_copy.update(session.extra_flags)
    flags_copy = self._filter_flags_for_run(flags_copy)
    return tuple(flags_copy.get_list())

  def _filter_flags_for_run(self, flags: Flags) -> Flags:
    return flags

  def quit(self, runner: Runner) -> None:
    del runner
    assert self._is_running
    try:
      self.force_quit()
    finally:
      self._pid = None

  def force_quit(self) -> None:
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
      runner: Runner,
      script: str,
      timeout: Optional[dt.timedelta] = None,
      arguments: Sequence[object] = ()
  ) -> Any:
    pass

  @abc.abstractmethod
  def show_url(self,
               runner: Runner,
               url: str,
               target: Optional[str] = None) -> None:
    pass

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
    return f"{platform_prefix}{self.type.capitalize()}:{self.label}"

  def __hash__(self) -> int:
    # Poor-man's hash, browsers should be unique.
    return hash(id(self))

  def performance_mark(self, runner: Runner, name: str):
    self.js(runner, "performance.mark(arguments[0]);", arguments=[name])
