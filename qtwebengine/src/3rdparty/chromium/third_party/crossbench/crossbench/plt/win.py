# Copyright 2023 The Chromium Authors
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

from __future__ import annotations

import functools
import os
import shutil
from typing import Optional

from crossbench import path as pth
from crossbench.plt.base import Platform


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
    return self.sh_stdout("wmic", "cpu", "get",
                          "name").strip().splitlines()[2].strip()

  def search_binary(self,
                    app_or_bin: pth.RemotePathLike) -> Optional[pth.RemotePath]:
    assert self.is_local, "Unsupported operation on remote platform"
    app_or_bin_path: pth.RemotePath = self.path(app_or_bin)
    if not app_or_bin_path.parts:
      raise ValueError("Got empty path")
    if app_or_bin_path.suffix != ".exe":
      raise ValueError("Expected executable path with '.exe' suffix, "
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

  def app_version(self, app_or_bin: pth.RemotePathLike) -> str:
    app_or_bin = self.path(app_or_bin)
    assert self.exists(app_or_bin), f"Binary {app_or_bin} does not exist."
    return self.sh_stdout(
        "powershell", "-command",
        f"(Get-Item '{app_or_bin}').VersionInfo.ProductVersion")

  def symlink_or_copy(self, src: pth.RemotePathLike,
                      dst: pth.RemotePathLike) -> pth.RemotePath:
    """Windows does not support symlinking without admin support.
    Copy files on windows but symlink everywhere else (see base Platform)."""
    assert self.is_local, "Unsupported operation on remote platform"
    dst_path = self.path(dst)
    shutil.copy(self.path(src), dst_path)
    return dst_path
