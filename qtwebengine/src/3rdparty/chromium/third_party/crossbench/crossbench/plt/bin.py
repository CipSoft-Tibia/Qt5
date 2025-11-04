# Copyright 2024 The Chromium Authors
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

from __future__ import annotations

import functools
from typing import TYPE_CHECKING, Optional

from crossbench import path as pth

if TYPE_CHECKING:
  from crossbench.plt.base import Platform


class BinaryNotFoundError(RuntimeError):

  def __init__(self, binary: Binary, platform: Platform):
    self.binary = binary
    self.platform = platform
    super().__init__(self._create_message())

  def _create_message(self) -> str:
    return (f"Could not find binary '{self.binary}' on {self.platform}. "
            f"Please install {self.binary.name} or use the "
            f"--bin-{self.binary.name} "
            "command line flag to manually specify a path.")


class UnsupportedPlatformError(BinaryNotFoundError):

  def __init__(self, binary: Binary, platform: Platform, expected: str):
    self.expected_platform_name: str = expected
    super().__init__(binary, platform)

  def _create_message(self) -> str:
    return (f"Could not find binary '{self.binary}' on {self.platform}. "
            f"Only supported on {self.expected_platform_name}")


class Binary:
  """A binary abstraction for multiple platforms.
  Use this implementation to define binaries that exist on multiple platforms.
  For platform-specific binaries use subclasses of Binary."""

  def __init__(self,
               name: str,
               default: Optional[pth.RemotePathLike] = None,
               posix: Optional[pth.RemotePathLike] = None,
               linux: Optional[pth.RemotePathLike] = None,
               android: Optional[pth.RemotePathLike] = None,
               macos: Optional[pth.RemotePathLike] = None,
               win: Optional[pth.RemotePathLike] = None) -> None:
    self._name = name
    self._default = self._convert(default)
    self._posix = self._convert(posix)
    self._linux = self._convert(linux)
    self._android = self._convert(android)
    self._macos = self._convert(macos)
    self._win = self._convert(win)
    if self._win and self._win.suffix != ".exe":
      raise ValueError(f"Windows binary {self._win} should have '.exe' suffix")
    if not any((default, posix, linux, android, macos, win)):
      raise ValueError("At least one platform binary must be provided")

  def _convert(self,
               path: Optional[pth.RemotePathLike]) -> Optional[pth.RemotePath]:
    if path is None:
      return None
    if not path:
      raise ValueError("Got unexpected empty string as binary path")
    return pth.RemotePath(path)

  @property
  def name(self) -> str:
    return self._name

  def __str__(self) -> str:
    return self._name

  @functools.lru_cache(maxsize=None)  # pylint: disable=method-cache-max-size-none
  def resolve_cached(self, platform: Platform) -> pth.RemotePath:
    return self.resolve(platform)

  def resolve(self, platform: Platform) -> pth.RemotePath:
    self._validate_platform(platform)
    if binary := self.platform_path(platform):
      binary_path = platform.path(binary)
      if result := platform.search_binary(binary_path):
        return result
    raise BinaryNotFoundError(self, platform)

  def platform_path(self, platform: Platform) -> Optional[pth.RemotePath]:
    if self._linux and platform.is_linux:
      return self._linux
    if self._android and platform.is_android:
      return self._android
    if self._macos and platform.is_macos:
      return self._macos
    if self._posix and platform.is_posix:
      return self._posix
    if platform.is_win:
      if self._win:
        return self._win
      if self._default:
        if self._default.suffix == ".exe":
          return self._default
        # Auto-append default
        return self._default.with_suffix(".exe")
    return self._default

  def _validate_platform(self, platform: Platform) -> None:
    pass


class PosixBinary(Binary):

  def __init__(self, name: pth.RemotePathLike):
    super().__init__(pth.RemotePath(name).name, posix=name)

  def _validate_platform(self, platform: Platform) -> None:
    if not platform.is_posix:
      raise UnsupportedPlatformError(self, platform, "posix")


class MacOsBinary(Binary):

  def __init__(self, name: pth.RemotePathLike):
    super().__init__(pth.RemotePath(name).name, macos=name)

  def _validate_platform(self, platform: Platform) -> None:
    if not platform.is_macos:
      raise UnsupportedPlatformError(self, platform, "macos")


class LinuxBinary(Binary):

  def __init__(self, name: pth.RemotePathLike):
    super().__init__(pth.RemotePath(name).name, linux=name)

  def _validate_platform(self, platform: Platform) -> None:
    if not platform.is_posix:
      raise UnsupportedPlatformError(self, platform, "linux")


class AndroidBinary(Binary):

  def __init__(self, name: pth.RemotePathLike):
    super().__init__(pth.RemotePath(name).name, android=name)

  def _validate_platform(self, platform: Platform) -> None:
    if not platform.is_android:
      raise UnsupportedPlatformError(self, platform, "android")


class WinBinary(Binary):

  def __init__(self, name: pth.RemotePathLike):
    super().__init__(pth.RemotePath(name).name, win=name)

  def _validate_platform(self, platform: Platform) -> None:
    if not platform.is_win:
      raise UnsupportedPlatformError(self, platform, "windows")


class Binaries:
  CPIO = LinuxBinary("cpio")
  FFMPEG = Binary("ffmpeg", posix="ffmpeg")
  GCERTSTATUS = Binary("gcertstatus", posix="gcertstatus")
  GO = Binary("go", posix="go")
  GSUTIL = Binary("gsutil", posix="gsutil")
  LSCPU = LinuxBinary("lscpu")
  MONTAGE = Binary("montage", posix="montage")
  ON_AC_POWER = LinuxBinary("on_ac_power")
  PERF = LinuxBinary("perf")
  PPROF = LinuxBinary("pprof")
  PYTHON3 = Binary("python3", default="python3", win="python3.exe")
  RPM2CPIO = LinuxBinary("rpm2cpio")
  SIMPLEPERF = AndroidBinary("simpleperf")
  XCTRACE = MacOsBinary("xctrace")
