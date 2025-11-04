# Copyright 2023 The Chromium Authors
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

from __future__ import annotations

import argparse
import dataclasses
import logging
import re
from typing import Any, Dict, Optional, TextIO, Tuple, cast

import hjson

import crossbench.browsers.all as browsers
from crossbench import cli_helper, exception
from crossbench import path as pth
from crossbench import plt
from crossbench.browsers.chrome.downloader import ChromeDownloader
from crossbench.browsers.firefox.downloader import FirefoxDownloader
from crossbench.cli.config.driver import BrowserDriverType, DriverConfig
from crossbench.cli.config.network import NetworkConfig, NetworkSpeedPreset
from crossbench.config import ConfigObject, ConfigParser

SUPPORTED_BROWSER = ("chromium", "chrome", "safari", "edge", "firefox")

# Split inputs like:
# - "/out/x64.release/chrome"
# - "/out/x64.release/chrome:4G"
# - "C:\out\x64.release\chrome"
# - "C:\out\x64.release\chrome:4G"
# - "applescript:/out/x64.release/chrome"
# - "applescript:/out/x64.release/chrome:4G"
# - "selenium:C:\out\x64.release\chrome"
# - "selenium:C:\out\x64.release\chrome:4G"
NETWORK_PRESETS: str = "|".join(
    re.escape(preset.value) for preset in NetworkSpeedPreset)  # pytype: disable=missing-parameter
SHORT_FORM_RE = re.compile(r"((?P<driver>\w{3,}):)??"
                           r"(?P<path>([A-Z]:[/\\])?[^:]+)"
                           f"(:(?P<network>{NETWORK_PRESETS}))?")
ANDROID_PACKAGE_RE = re.compile(r"[a-z]+(\.[a-z]+){2,}")
VERSION_FOR_RANGE_RE = re.compile(r"(?P<prefix>[^\d]*)(?P<milestone>\d+)")


@dataclasses.dataclass(frozen=True)
class BrowserConfig(ConfigObject):
  browser: pth.RemotePathLike
  driver: DriverConfig = DriverConfig.default()
  # Make network optional since --network provides a global default and we do
  # want to have the option to explicitly specify the default network in a
  # browser config.
  network: Optional[NetworkConfig] = None

  def __post_init__(self) -> None:
    if not self.browser:
      raise ValueError(f"{type(self).__name__}.browser cannot be None.")
    if not self.driver:
      raise ValueError(f"{type(self).__name__}.driver cannot be None.")

  @classmethod
  def default(cls) -> BrowserConfig:
    return cls(
        browsers.Chrome.stable_path(plt.PLATFORM), DriverConfig.default())

  @classmethod
  def parse_str(cls, value: str) -> BrowserConfig:
    if not value:
      raise argparse.ArgumentTypeError("Cannot parse empty string")
    network: Optional[NetworkConfig] = None
    driver = DriverConfig.default()
    path: Optional[pth.RemotePathLike] = None
    if ":" not in value or cls.value_has_path_prefix(value):
      # Variant 1: $PATH_OR_IDENTIFIER
      path = cls._parse_path_or_identifier(value)
    elif value[0] != "{":
      # Variant 2: ${DRIVER_TYPE}:${PATH_OR_IDENTIFIER}:${NETWORK}
      driver, path, network = cls._parse_inline_short_form(value)
    else:
      # Variant 3: Full inline hjson
      return cls.parse_inline_hjson(value)
    assert path, "Invalid path"
    return cls(path, driver, network)

  @classmethod
  def parse_with_range(cls, value: Any) -> Tuple[BrowserConfig, ...]:
    if isinstance(value, str):
      return cls._parse_with_range(value)
    return (cls.parse(value),)

  @classmethod
  def _parse_with_range(cls, value: str) -> Tuple[BrowserConfig, ...]:
    if not value:
      raise argparse.ArgumentTypeError("Cannot parse empty string")
    parts = value.split("...", maxsplit=1)
    start_version: str = parts.pop(0)
    if not parts:
      return (cls.parse(start_version),)
    limit_version = parts[0]

    start_match = VERSION_FOR_RANGE_RE.fullmatch(start_version)
    if not start_match:
      raise argparse.ArgumentTypeError(
          f"Start of a browser range {repr(value)} must end in digits, "
          f"but got {repr(start_version)}")
    limit_match = VERSION_FOR_RANGE_RE.fullmatch(limit_version)
    if not limit_match:
      raise argparse.ArgumentTypeError(
          f"Upper limit of a browser range {repr(value)} must end in digits, "
          f"but got {repr(limit_version)}")

    start_prefix = start_match["prefix"]
    limit_prefix = limit_match["prefix"]
    if limit_prefix and not start_prefix.endswith(limit_prefix):
      raise argparse.ArgumentTypeError(
          f"Browser version range start prefix {repr(start_prefix)} must match "
          f"limit prefix {repr(limit_prefix)}: {repr(value)}")

    start_milestone: int = cli_helper.parse_positive_int(
        start_match["milestone"], "browser version range start milestone")
    limit_milestone: int = cli_helper.parse_positive_int(
        limit_match["milestone"], "browser version range limit milestone")
    if start_milestone > limit_milestone:
      raise argparse.ArgumentTypeError(
          f"Browser version limit must be larger than start: {repr(value)}")

    count = limit_milestone - start_milestone
    logging.info("Creating %d intermediate browser versions from %s", count,
                 value)
    versions = []
    for milestone in range(start_milestone, limit_milestone + 1):
      version_str = f"{start_prefix}{milestone}"
      versions.append(cls.parse(version_str))
    return tuple(versions)

  @classmethod
  def _parse_path_or_identifier(
      cls,
      maybe_path_or_identifier: str,
      driver_type: Optional[BrowserDriverType] = None,
      driver: Optional[DriverConfig] = None) -> pth.RemotePathLike:
    if not maybe_path_or_identifier:
      raise argparse.ArgumentTypeError("Got empty browser identifier.")
    if not driver_type:
      if driver:
        driver_type = driver.type
      else:
        driver_type = BrowserDriverType.default()
    identifier = maybe_path_or_identifier.lower()
    path = None
    if "/" in maybe_path_or_identifier or "\\" in maybe_path_or_identifier:
      if cls._is_downloadable_identifier(maybe_path_or_identifier):
        return maybe_path_or_identifier
      # Assume a path since short-names never contain back-/slashes.
      if driver_type.is_remote:
        path = cli_helper.parse_path(maybe_path_or_identifier)
      else:
        path = cli_helper.parse_existing_path(maybe_path_or_identifier)
    else:
      if ":" in maybe_path_or_identifier:
        raise argparse.ArgumentTypeError(
            "Got unexpected short-form string "
            f"{repr(maybe_path_or_identifier)}. \n"
            "  - Use a complex browser config with separate "
            "'browser' and 'driver' attributes, or\n"
            "  - Use the short-form directly on the parent config attribute: \n"
            f"   {{my-browser: '{maybe_path_or_identifier}'}}")
      if maybe_path := cls._try_parse_short_name(identifier, driver_type):
        return maybe_path
      if cls._is_downloadable_identifier(maybe_path_or_identifier):
        return maybe_path_or_identifier
      if driver_type == BrowserDriverType.ANDROID:
        if ANDROID_PACKAGE_RE.fullmatch(maybe_path_or_identifier):
          return pth.RemotePath(maybe_path_or_identifier)
    if not path:
      path = cli_helper.try_resolve_existing_path(maybe_path_or_identifier)
      if not path:
        raise argparse.ArgumentTypeError(
            f"Unknown browser path or short name: '{maybe_path_or_identifier}'")
    if cls.is_supported_browser_path(path):
      return path
    raise argparse.ArgumentTypeError(f"Unsupported browser path='{path}'")

  @classmethod
  def _is_downloadable_identifier(cls, maybe_path_or_identifier: str) -> bool:
    # TODO: handle remote platforms.
    platform = plt.PLATFORM
    if ChromeDownloader.is_valid(maybe_path_or_identifier, platform):
      return True
    if FirefoxDownloader.is_valid(maybe_path_or_identifier, platform):
      return True
    return False

  @classmethod
  def _try_parse_short_name(
      cls, identifier: str,
      driver_type: BrowserDriverType) -> Optional[pth.RemotePath]:
    # We're not using a dict-based lookup here, since not all browsers are
    # available on all platforms
    # TODO: handle remote platforms.
    platform = plt.PLATFORM
    if identifier in ("chrome", "chrome-stable", "chr-stable", "chr"):
      if driver_type == BrowserDriverType.ANDROID:
        return pth.RemotePath("com.android.chrome")
      return browsers.Chrome.stable_path(platform)
    if identifier in ("chrome-app"):
      if driver_type == BrowserDriverType.ANDROID:
        return pth.RemotePath("com.google.android.apps.chrome")
    if identifier in ("chrome-beta", "chr-beta"):
      if driver_type == BrowserDriverType.ANDROID:
        return pth.RemotePath("com.chrome.beta")
      return browsers.Chrome.beta_path(platform)
    if identifier in ("chrome-dev", "chr-dev"):
      if driver_type == BrowserDriverType.ANDROID:
        return pth.RemotePath("com.chrome.dev")
      return browsers.Chrome.dev_path(platform)
    if identifier in ("chrome-canary", "chr-canary"):
      if driver_type == BrowserDriverType.ANDROID:
        return pth.RemotePath("com.chrome.canary")
      return browsers.Chrome.canary_path(platform)
    if identifier == "chromium":
      if driver_type == BrowserDriverType.ANDROID:
        return pth.RemotePath("org.chromium.chrome")
      return browsers.Chromium.default_path(platform)
    if identifier in ("edge", "edge-stable"):
      return browsers.Edge.stable_path(platform)
    if identifier == "edge-beta":
      return browsers.Edge.beta_path(platform)
    if identifier == "edge-dev":
      return browsers.Edge.dev_path(platform)
    if identifier == "edge-canary":
      return browsers.Edge.canary_path(platform)
    if identifier in ("safari", "sf", "safari-stable", "sf-stable"):
      return browsers.Safari.default_path(platform)
    if identifier in ("safari-technology-preview", "safari-tp", "sf-tp", "tp"):
      return browsers.Safari.technology_preview_path(platform)
    if identifier in ("firefox", "firefox-stable", "ff", "ff-stable"):
      return browsers.Firefox.default_path(platform)
    if identifier in ("firefox-dev", "firefox-developer-edition", "ff-dev"):
      return browsers.Firefox.developer_edition_path(platform)
    if identifier in ("firefox-nightly", "ff-nightly", "ff-trunk"):
      return browsers.Firefox.nightly_path(platform)
    return None

  @classmethod
  def is_supported_browser_path(cls, path: pth.RemotePath) -> bool:
    path_str = str(path).lower()
    for short_name in SUPPORTED_BROWSER:
      if short_name in path_str:
        return True
    return False

  @classmethod
  def _parse_inline_short_form(
      cls, value: str
  ) -> Tuple[DriverConfig, pth.RemotePathLike, Optional[NetworkConfig]]:
    assert ":" in value
    match = SHORT_FORM_RE.fullmatch(value)
    if not match:
      raise argparse.ArgumentTypeError(
          f"Invalid browser short form: '{value}' \n"
          "A browser path/identifier and "
          "at least a driver or network preset have to be present")
    driver_identifier = match.group("driver")
    path_or_identifier = match.group("path")
    network_identifier = match.group("network")
    if not path_or_identifier:
      raise argparse.ArgumentTypeError(
          "Browser short form: missing path or browser identifier.")
    driver = DriverConfig.default()
    if driver_identifier is not None:
      driver = cast(DriverConfig, DriverConfig.parse(match.group("driver")))
    path: pth.RemotePathLike = cls._parse_path_or_identifier(
        path_or_identifier, driver.type)
    network = None
    if network_identifier is not None:
      network = NetworkConfig.parse_str(network_identifier)
    return (driver, path, network)

  @classmethod
  def parse_text_io(cls, f: TextIO) -> BrowserConfig:
    with exception.annotate(f"Loading browser config file: {f.name}"):
      config = {}
      with exception.annotate(f"Parsing {hjson.__name__}"):
        config = hjson.load(f)
      with exception.annotate(f"Parsing config file: {f.name}"):
        return cls.parse_dict(config)
    raise argparse.ArgumentTypeError(f"Could not parse : '{f.name}'")

  @classmethod
  def parse_dict(cls, config: Dict[str, Any]) -> BrowserConfig:
    return cls.config_parser().parse(config)

  @classmethod
  def config_parser(cls) -> ConfigParser[BrowserConfig]:
    parser = ConfigParser("BrowserConfig parser", cls)
    parser.add_argument(
        "browser",
        aliases=("path",),
        type=cls._parse_path_or_identifier,
        required=True,
        depends_on=("driver",))
    parser.add_argument(
        "driver", type=DriverConfig, default=DriverConfig.default())
    parser.add_argument(
        "network", required=False, default=None, type=NetworkConfig)
    return parser

  @property
  def path(self) -> pth.RemotePath:
    assert isinstance(self.browser, pth.RemotePath)
    return self.browser

  def get_platform(self) -> plt.Platform:
    return self.driver.get_platform()
