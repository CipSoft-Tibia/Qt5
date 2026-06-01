# Copyright 2024 The Chromium Authors
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

from __future__ import annotations

import argparse
import logging
import re
from typing import TYPE_CHECKING, Optional, TextIO, Tuple, cast

from crossbench import path as pth
from crossbench.browsers.browser import Browser
from crossbench.browsers.browser_helper import convert_flags_to_label
from crossbench.browsers.viewport import Viewport
from crossbench.flags.chrome import ChromeFlags
from crossbench.types import JsonDict

if TYPE_CHECKING:
  from crossbench.browsers.settings import Settings
  from crossbench.flags.base import Flags, FlagsData
  from crossbench.flags.chrome import ChromeFeatures
  from crossbench.flags.js_flags import JSFlags
  from crossbench.runner.groups.session import BrowserSessionRunGroup


class ChromiumBased(Browser):
  MIN_HEADLESS_NEW_VERSION: int = 112
  DEFAULT_FLAGS: Tuple[str, ...] = (
      "--no-default-browser-check",
      "--disable-component-update",
      "--disable-sync",
      "--disable-extensions",
      "--no-first-run",
      # This could be enabled via feature-flags as well.
      "--disable-search-engine-choice-screen",
  )
  FLAGS_FOR_DISABLING_BACKGROUND_INTERVENTIONS: Tuple[str, ...] = (
      "--disable-background-timer-throttling",
      "--disable-renderer-backgrounding",
  )

  @classmethod
  def default_flags(cls, initial_data: FlagsData = None) -> ChromeFlags:
    return ChromeFlags(initial_data)

  def __init__(self,
               label: str,
               path: pth.AnyPath,
               settings: Optional[Settings] = None):
    super().__init__(label, path, settings=settings)
    self._stdout_log_file: Optional[TextIO] = None
    assert isinstance(self._flags, ChromeFlags)

  def _setup_flags(self, settings: Settings) -> ChromeFlags:
    flags: Flags = settings.flags
    js_flags: Flags = settings.js_flags
    self._flags = self.default_flags(self.DEFAULT_FLAGS)
    self._flags.update(flags)

    if "--allow-background-interventions" in self._flags.data:
      # The --allow-background-interventions flag should have no value.
      assert self._flags.get("--allow-background-interventions") is None
    else:
      self._flags.update(self.FLAGS_FOR_DISABLING_BACKGROUND_INTERVENTIONS)

    # Explicitly disable field-trials by default on all chrome flavours:
    # By default field-trials are disabled on non-Chrome branded builds, but
    # are auto-enabled on everything else. This gives very confusing results
    # when comparing local builds to official binaries.
    field_trial_flags: ChromeFlags = self._flags.field_trial_flags
    if not field_trial_flags:
      logging.info("Disabling experiments/finch/field-trials for %s", self)
      for flag in ChromeFlags.NO_EXPERIMENTS_FLAGS:
        self._flags.set(flag)
    else:
      logging.warning("Running with field-trials or finch experiments.")

    self.js_flags.update(js_flags)
    self._maybe_disable_gpu_compositing()
    # Run early validation for conflicting command-line flags.
    self.validate_flags()
    return self._flags

  def _maybe_disable_gpu_compositing(self) -> None:
    # Chrome Remote Desktop provides no GPU and older chrome versions
    # don't handle this well.
    if self.major_version > 92 or ("CHROME_REMOTE_DESKTOP_SESSION"
                                   not in self.platform.environ):
      return
    self.flags.set("--disable-gpu-compositing")
    self.flags.set("--no-sandbox")

  def validate_flags(self) -> None:
    super().validate_flags()
    field_trial_flags: ChromeFlags = self.flags.field_trial_flags
    no_finch_flags = self.flags.no_experiments_flags
    if field_trial_flags and no_finch_flags:
      raise argparse.ArgumentTypeError(
          f"Conflicting {self.type_name} flags detected: "
          f"{field_trial_flags} vs {no_finch_flags}.\n"
          "Cannot enable and disable finch / field-trials at the same time.")

  def _setup_cache_dir(self, settings: Settings) -> None:
    cache_dir = settings.cache_dir
    if cache_dir is None:
      maybe_cache_dir = self._flags.get("--user-data-dir", None)
      if maybe_cache_dir:
        cache_dir = pth.AnyPath(maybe_cache_dir)
    if cache_dir is None:
      self.cache_dir = self.platform.mkdtemp(prefix=self.type_name)
      self.clear_cache_dir = True
    else:
      self.cache_dir = cache_dir
      self.clear_cache_dir = False

  def _extract_version(self) -> str:
    assert self.path
    version_string = self.platform.app_version(self.path)
    # Sample output: "Chromium 90.0.4430.212 dev" => "90.0.4430.212"
    matches = re.findall(r"[\d.]+", version_string)
    if not matches:
      raise ValueError(
          f"Could not extract version number from '{version_string}' "
          f"for '{self.path}'")
    return str(matches[0])

  @property
  def is_headless(self) -> bool:
    return "--headless" in self._flags

  @property
  def chrome_log_file(self) -> pth.AnyPath:
    assert self.log_file
    return self.log_file.with_suffix(f".{self.type_name}.log")

  @property
  def flags(self) -> ChromeFlags:
    return cast(ChromeFlags, self._flags)

  @property
  def js_flags(self) -> JSFlags:
    return cast(ChromeFlags, self._flags).js_flags

  @property
  def features(self) -> ChromeFeatures:
    return cast(ChromeFlags, self._flags).features

  def details_json(self) -> JsonDict:
    details: JsonDict = super().details_json()
    if self.log_file:
      log = cast(JsonDict, details["log"])
      log[self.type_name] = str(self.chrome_log_file)
      log["stdout"] = str(self.stdout_log_file)
    details["js_flags"] = tuple(self.js_flags)
    return details

  def _get_browser_flags_for_session(
      self, session: BrowserSessionRunGroup) -> Tuple[str, ...]:
    js_flags_copy = self.js_flags.copy()
    js_flags_copy.update(session.extra_js_flags)

    flags_copy = self.flags.copy()
    flags_copy.update(session.extra_flags)
    flags_copy.update(self.network.extra_flags(self.attributes))
    self._handle_viewport_flags(flags_copy)

    if len(js_flags_copy):
      flags_copy["--js-flags"] = str(js_flags_copy)
    if user_data_dir := self.flags.get("--user-data-dir"):
      assert user_data_dir == str(
          self.cache_dir), (f"--user-data-dir path: {user_data_dir} was passed "
                            f"but does not match cache-dir: {self.cache_dir}")
    if self.cache_dir:
      flags_copy["--user-data-dir"] = str(self.cache_dir)
    if self.log_file:
      flags_copy.set("--enable-logging")
      flags_copy["--log-file"] = str(self.chrome_log_file)

    flags_copy = self._filter_flags_for_run(flags_copy)

    return tuple(flags_copy)

  def _handle_viewport_flags(self, flags: Flags) -> None:
    self._sync_viewport_flag(flags, "--start-fullscreen",
                             self.viewport.is_fullscreen, Viewport.FULLSCREEN)
    self._sync_viewport_flag(flags, "--start-maximized",
                             self.viewport.is_maximized, Viewport.MAXIMIZED)
    self._sync_viewport_flag(flags, "--headless", self.viewport.is_headless,
                             Viewport.HEADLESS)
    # M112 added --headless=new as replacement for --headless
    if "--headless" in flags and (self.major_version
                                  >= self.MIN_HEADLESS_NEW_VERSION):
      if flags["--headless"] is None:
        logging.info("Replacing --headless with --headless=new")
        flags.set("--headless", "new", override=True)

    if self.viewport.is_default:
      update_viewport = False
      width, height = self.viewport.size
      x, y = self.viewport.position
      if "--window-size" in flags:
        update_viewport = True
        width, height = map(int, flags["--window-size"].split(","))
      if "--window-position" in flags:
        update_viewport = True
        x, y = map(int, flags["--window-position"].split(","))
      if update_viewport:
        self.viewport = Viewport(width, height, x, y)
    if self.viewport.has_size:
      flags["--window-size"] = f"{self.viewport.width},{self.viewport.height}"
      flags["--window-position"] = f"{self.viewport.x},{self.viewport.y}"
    else:
      for flag in ("--window-position", "--window-size"):
        if flag in flags:
          flag_value = flags[flag]
          raise ValueError(f"Viewport {self.viewport} conflicts with flag "
                           f"{flag}={flag_value}")

  def get_label_from_flags(self) -> str:
    return convert_flags_to_label(*self.flags, *self.js_flags)

  def quit(self) -> None:
    super().quit()
    if self._stdout_log_file:
      self._stdout_log_file.close()
      self._stdout_log_file = None
