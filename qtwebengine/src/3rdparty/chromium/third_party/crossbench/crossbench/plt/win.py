# Copyright 2023 The Chromium Authors
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

from __future__ import annotations

import functools
import logging
import os
import shutil
from typing import Optional, Type

from crossbench import path as pth
from crossbench.plt.base import Platform
from crossbench.plt.signals import WinSignals


class WinPlatform(Platform):
  # TODO: support remote platforms
  SEARCH_PATHS = (
      pth.LocalPath("."),
      pth.LocalPath(os.path.expandvars("%ProgramFiles%")),
      pth.LocalPath(os.path.expandvars("%ProgramFiles(x86)%")),
      pth.LocalPath(os.path.expandvars("%APPDATA%")),
      pth.LocalPath(os.path.expandvars("%LOCALAPPDATA%")),
  )

  @property
  def signals(self) -> Type[WinSignals]:
    return WinSignals

  @property
  def is_win(self) -> bool:
    return True

  @property
  def name(self) -> str:
    return "win"

  @property
  def device(self) -> str:
    # TODO: implement
    return ""

  @functools.cached_property
  def version(self) -> str:  #pylint: disable=invalid-overridden-method
    return self.sh_stdout("cmd", "/c", "ver").strip()

  @functools.cached_property
  def cpu(self) -> str:  #pylint: disable=invalid-overridden-method
    return self.sh_stdout(
      "powershell", "-c",
      "Get-CIMInstance -query 'select * from Win32_Processor' | ft Name"
    ).strip().splitlines()[2].strip()

  def search_binary(self, app_or_bin: pth.AnyPathLike) -> Optional[pth.AnyPath]:
    self.assert_is_local()
    app_or_bin_path: pth.AnyPath = self.path(app_or_bin)
    if not app_or_bin_path.parts:
      raise ValueError("Got empty path")
    if app_or_bin_path.suffix.lower() not in (".exe", ".bat"):
      raise ValueError("Expected executable path with '.exe' or '.bat' suffix, "
                       f"but got: '{app_or_bin_path.name}'")
    if result_path := self.which(app_or_bin):
      assert self.exists(result_path), f"{result_path} does not exist."
      return result_path
    for path in self.SEARCH_PATHS:
      # Recreate Path object for easier pyfakefs testing
      result_path = self.path(path) / app_or_bin
      if self.exists(result_path):
        return result_path
    return None

  def app_version(self, app_or_bin: pth.AnyPathLike) -> str:
    app_or_bin = self.path(app_or_bin)
    if not self.exists(app_or_bin):
      raise ValueError(f"Binary {app_or_bin} does not exist.")
    if version := self.sh_stdout(
        "powershell", "-command",
        f"(Get-Item '{app_or_bin}').VersionInfo.ProductVersion").strip():
      name = self.sh_stdout(
          "powershell", "-command",
          f"(Get-Item '{app_or_bin}').VersionInfo.ProductName").strip()
      return f"{name} {version}"
    try:
      # Fall back to command-line tools.
      if version := self.sh_stdout(app_or_bin, "--version").strip():
        return version
    except Exception as e:  # pylint: disable=broad-exception-caught
      logging.debug("Failed to extract binary tool version: %s", e)
    raise ValueError(f"Could not extract version for {app_or_bin}")


  def symlink_or_copy(self, src: pth.AnyPathLike,
                      dst: pth.AnyPathLike) -> pth.AnyPath:
    """Windows does not support symlinking without admin support.
    Copy files on windows but symlink everywhere else (see base Platform)."""
    self.assert_is_local()
    dst_path = self.path(dst)
    shutil.copy(os.fspath(self.path(src)), os.fspath(dst_path))
    return dst_path
