# Copyright 2023 The Chromium Authors
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

from __future__ import annotations

import logging
from typing import TYPE_CHECKING, Optional

from crossbench import compat
from crossbench import path as pth
from crossbench.browsers.attributes import BrowserAttributes
from crossbench.browsers.browser import Browser

if TYPE_CHECKING:
  from crossbench import plt
  from crossbench.browsers.settings import Settings
  from crossbench.runner.runner import Runner


SAFARIDRIVER_PATH = pth.RemotePath("/usr/bin/safaridriver")


def find_safaridriver(bin_path: pth.RemotePath,
                      platform: plt.Platform) -> pth.RemotePath:
  assert platform.is_file(bin_path), f"Invalid binary path: {bin_path}"
  driver_path = bin_path.parent / "safaridriver"
  if platform.exists(driver_path):
    return driver_path
  # The system-default Safari version doesn't come with the driver
  assert compat.is_relative_to(bin_path, Safari.default_path(platform)), (
      f"Expected default Safari.app binary but got {bin_path}")
  return SAFARIDRIVER_PATH


class Safari(Browser):

  @classmethod
  def default_path(cls, platform: plt.Platform) -> pth.RemotePath:
    return platform.path("/Applications/Safari.app")

  @classmethod
  def technology_preview_path(cls, platform: plt.Platform) -> pth.RemotePath:
    return platform.path("/Applications/Safari Technology Preview.app")

  def __init__(self,
               label: str,
               path: pth.RemotePath,
               settings: Optional[Settings] = None):
    super().__init__(label, path, settings=settings)
    assert self.platform.is_macos, "Safari only works on MacOS"
    self.bundle_name: str = ""

  def _setup_path(self, path: Optional[pth.RemotePath] = None) -> None:
    super()._setup_path(path)
    assert self.path
    self.bundle_name = self.path.stem.replace(" ", "")

  def _setup_cache_dir(self, settings: Settings) -> None:
    assert settings.cache_dir is None, (
        "Cannot set custom cache dir for Safari")
    assert self.bundle_name, "Missing bundle_name"
    self.cache_dir = self.platform.home() / (
        f"Library/Containers/com.apple.{self.bundle_name}/Data/Library/Caches")

  @property
  def type_name(self) -> str:
    return "safari"

  @property
  def attributes(self) -> BrowserAttributes:
    return BrowserAttributes.SAFARI

  def clear_cache(self, runner: Runner) -> None:
    self._clear_cache()

  def _clear_cache(self) -> None:
    logging.info("CLEAR CACHE: %s", self)
    self.platform.exec_apple_script(f"""
      tell application "{self.app_path}" to activate
      tell application "System Events"
          keystroke "e" using {{command down, option down}}
      end tell""")

  def _extract_version(self) -> str:
    # Use the shipped safaridriver to get the more detailed version
    # TODO: support remote platform
    driver_version = self.platform.app_version(
        find_safaridriver(self.path, self.platform))
    # Input: "Included with Safari 16.6 (18615.3.6.11.1)"
    # Output: " (18615.3.6.11.1)"
    driver_version = " (" + driver_version.split(" (", maxsplit=1)[1]
    assert self.path
    app_path = self.path.parents[2]
    return self.platform.app_version(app_path) + driver_version
