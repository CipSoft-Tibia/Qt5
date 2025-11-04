# Copyright 2023 The Chromium Authors
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

from __future__ import annotations

import functools
import os
from typing import Any, Dict, Optional, Tuple

from crossbench import path as pth
from crossbench.plt.base import SubprocessError
from crossbench.plt.posix import PosixPlatform
from crossbench.plt.remote import RemotePlatformMixin


class LinuxPlatform(PosixPlatform):
  SEARCH_PATHS: Tuple[pth.RemotePath, ...] = (
      pth.RemotePath("."),
      pth.RemotePath("/usr/local/sbin"),
      pth.RemotePath("/usr/local/bin"),
      pth.RemotePath("/usr/sbin"),
      pth.RemotePath("/usr/bin"),
      pth.RemotePath("/sbin"),
      pth.RemotePath("/bin"),
      pth.RemotePath("/opt/google"),
  )

  @property
  def is_linux(self) -> bool:
    return True

  @property
  def name(self) -> str:
    return "linux"

  def check_system_monitoring(self, disable: bool = False) -> bool:
    return True

  @functools.cached_property
  def device(self) -> str:  #pylint: disable=invalid-overridden-method
    try:
      id_dir = self.path("/sys/devices/virtual/dmi/id")
      vendor = self.cat(id_dir / "sys_vendor").strip()
      product = self.cat(id_dir / "product_name").strip()
      return f"{vendor} {product}"
    except (FileNotFoundError, SubprocessError):
      return "UNKNOWN"

  @functools.cached_property
  def cpu(self) -> str:  #pylint: disable=invalid-overridden-method
    cpu_str = "UNKNOWN"
    for line in self.cat(self.path("/proc/cpuinfo")).splitlines():
      if line.startswith("model name"):
        _, cpu_str = line.split(":", maxsplit=2)
        break
    if cores_info := self._get_cpu_cores_info():
      cpu_str = f"{cpu_str} {cores_info}"
    return cpu_str

  @property
  def has_display(self) -> bool:
    return "DISPLAY" in os.environ

  @property
  def is_battery_powered(self) -> bool:
    if self.is_local:
      return super().is_battery_powered
    if self.which("on_ac_power"):
      return self.sh("on_ac_power", check=False).returncode == 1
    return False

  def system_details(self) -> Dict[str, Any]:
    details = super().system_details()
    for info_bin in ("lscpu", "inxi"):
      if self.which(info_bin):
        details[info_bin] = self.sh_stdout(info_bin)
    return details

  def search_binary(self,
                    app_or_bin: pth.RemotePathLike) -> Optional[pth.RemotePath]:
    app_or_bin_path: pth.RemotePath = self.path(app_or_bin)
    if not app_or_bin_path.parts:
      raise ValueError("Got empty path")
    if result_path := self.which(app_or_bin_path):
      if not self.exists(result_path):
        raise RuntimeError(f"{result_path} does not exist.")
      return result_path
    for path in self.SEARCH_PATHS:
      # Recreate Path object for easier pyfakefs testing
      result_path = self.path(path) / app_or_bin_path
      if self.exists(result_path):
        return result_path
    return None

  def screenshot(self, result_path: pth.RemotePath) -> None:
    # TODO: maybe use imagemagick's 'import' as more portable alternative
    self.sh("gnome-screenshot", "--file", result_path)


class RemoteLinuxPlatform(RemotePlatformMixin, LinuxPlatform):
  pass
