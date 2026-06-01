# Copyright 2023 The Chromium Authors
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

from __future__ import annotations

import logging
import re
from typing import TYPE_CHECKING, Optional

from crossbench import compat
from crossbench import path as pth
from crossbench.browsers.attributes import BrowserAttributes
from crossbench.browsers.browser import Browser

if TYPE_CHECKING:
  from crossbench import plt
  from crossbench.browsers.settings import Settings


SAFARIDRIVER_PATH = pth.AnyPosixPath("/usr/bin/safaridriver")


def find_safaridriver(bin_path: pth.AnyPath,
                      platform: plt.Platform) -> pth.AnyPath:
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
  def default_path(cls, platform: plt.Platform) -> pth.AnyPath:
    return platform.path("/Applications/Safari.app")

  @classmethod
  def technology_preview_path(cls, platform: plt.Platform) -> pth.AnyPath:
    return platform.path("/Applications/Safari Technology Preview.app")

  def __init__(self,
               label: str,
               path: pth.AnyPath,
               settings: Optional[Settings] = None):
    self._type_name: str = "safari"
    # TODO: use version object
    settings = settings or Settings()
    if path == self.technology_preview_path(settings.platform):
      # Use a custom type name since safari tech-preview does not have
      # a different major version.
      self._type_name = "safari_tp"
    super().__init__(label, path, settings=settings)
    assert self.platform.is_macos, "Safari only works on MacOS"
    self.bundle_name: str = ""

  def _setup_path(self, path: Optional[pth.AnyPath] = None) -> None:
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
    return self._type_name

  @property
  def attributes(self) -> BrowserAttributes:
    return BrowserAttributes.SAFARI

  def clear_cache(self) -> None:
    logging.info("CLEAR CACHE: %s", self)
    assert self.cache_dir, "Missing cache dir"
    if not self.platform.exists(self.cache_dir.parent):
      logging.warning("Could not find existing config dir for %s.", self)
      return
    self.platform.rm(self.cache_dir, dir=True, missing_ok=True)
    # This magic wait lowers safaridriver startup failures.
    self.platform.sleep(0.5)

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
    browser_version = str(
        re.findall(r"[\d\.]+", self.platform.app_version(app_path))[0])
    return browser_version + driver_version
