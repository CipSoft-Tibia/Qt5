# Copyright 2024 The Chromium Authors
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

from __future__ import annotations

from typing import TYPE_CHECKING, Dict, Optional

from crossbench import path as pth
from crossbench import plt
from crossbench.browsers.secrets import SecretsConfig, Secret, SecretType
from crossbench.browsers.splash_screen import SplashScreen
from crossbench.browsers.viewport import Viewport
from crossbench.flags.base import Flags
from crossbench.flags.chrome import ChromeFlags
from crossbench.network.live import LiveNetwork

if TYPE_CHECKING:
  from crossbench.network.base import Network


class Settings:
  """Container object for browser agnostic settings."""

  def __init__(self,
               flags: Optional[Flags.InitialDataType] = None,
               js_flags: Optional[Flags.InitialDataType] = None,
               cache_dir: Optional[pth.RemotePath] = None,
               network: Optional[Network] = None,
               driver_path: Optional[pth.RemotePath] = None,
               viewport: Optional[Viewport] = None,
               splash_screen: Optional[SplashScreen] = None,
               platform: Optional[plt.Platform] = None,
               secrets: Optional[SecretsConfig] = None):
    self._flags = self._convert_flags(flags, "flags")
    self._js_flags = self._extract_js_flags(self._flags, js_flags)
    self._cache_dir = cache_dir
    self._platform = platform or plt.PLATFORM
    self._driver_path = driver_path
    self._network: Network = network or LiveNetwork()
    self._viewport: Viewport = viewport or Viewport.DEFAULT
    self._splash_screen: SplashScreen = splash_screen or SplashScreen.DEFAULT
    self._secrets: Dict[
        SecretType, Secret] = secrets.secrets if secrets is not None else {}

  def _extract_js_flags(self, flags: Flags,
                        js_flags: Optional[Flags.InitialDataType]) -> Flags:
    if isinstance(flags, ChromeFlags):
      chrome_js_flags = flags.js_flags
      if not js_flags:
        return chrome_js_flags
      if chrome_js_flags:
        raise ValueError(
            f"Ambiguous js-flags: flags.js_flags={repr(chrome_js_flags)}, "
            f"js_flags={repr(js_flags)}")
    return self._convert_flags(js_flags, "--js-flags")

  def _convert_flags(self, flags: Optional[Flags.InitialDataType],
                     label: str) -> Flags:
    if isinstance(flags, str):
      raise ValueError(f"{label} should be a list, but got: {repr(flags)}")
    if not flags:
      return Flags()
    if isinstance(flags, Flags):
      return flags
    return Flags(flags)

  @property
  def flags(self) -> Flags:
    return self._flags

  @property
  def js_flags(self) -> Flags:
    return self._js_flags

  @property
  def cache_dir(self) -> Optional[pth.RemotePath]:
    return self._cache_dir

  @property
  def driver_path(self) -> Optional[pth.RemotePath]:
    return self._driver_path

  @property
  def platform(self) -> plt.Platform:
    return self._platform

  @property
  def network(self) -> Network:
    return self._network

  @property
  def secrets(self) -> Dict[SecretType, Secret]:
    return self._secrets

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
