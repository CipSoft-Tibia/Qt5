# Copyright 2023 The Chromium Authors
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

from __future__ import annotations

import re
from typing import Dict, Final, Optional, Tuple

from crossbench.browsers.version import BrowserVersion, BrowserVersionChannel


class FirefoxVersion(BrowserVersion):
  _PARTS_LEN: Final[int] = 4
  _PREFIX_RE = re.compile(r"(mozilla )?(ff|firefox)[ -]?", re.I)
  _VERSION_RE = re.compile(r"(?P<prefix>[^\d]*)"
                           r"(?P<version>"
                           r"(?P<parts>\d+\.\d+(?P<channel>[ab.])\d+)"
                           r") ?(?P<channel_long>esr|any)?")
  _SPLIT_RE = re.compile(r"[ab.]")
  _CHANNEL_LOOKUP: Dict[str, BrowserVersionChannel] = {
      "esr": BrowserVersionChannel.LTS,
      ".": BrowserVersionChannel.STABLE,
      # IRL Firefox version numbers do not distinct beta from stable, so we
      # remap Firefox Dev => beta.
      "b": BrowserVersionChannel.BETA,
      "a": BrowserVersionChannel.ALPHA,
      "any": BrowserVersionChannel.ANY,
  }

  @classmethod
  def _parse(
      cls,
      full_version: str) -> Tuple[Tuple[int, ...], BrowserVersionChannel, str]:
    matches = cls._VERSION_RE.fullmatch(full_version.strip())
    if not matches:
      raise cls.parse_error("Could not extract version number", full_version)
    prefix = matches["prefix"]
    if not cls._validate_prefix(prefix):
      raise cls.parse_error(f"Wrong prefix {repr(prefix)}", full_version)
    version_str = matches["version"]
    version_parts = matches["parts"]
    assert version_parts and version_str
    if matches["channel_long"] and matches["channel"] != ".":
      raise cls.parse_error("Invalid ESR/Any channel version", full_version)
    browser_channel = cls._parse_channel(matches)
    parts = tuple(map(int, cls._SPLIT_RE.split(version_parts)))
    if len(parts) != 3:
      raise cls.parse_error("Invalid number of version number parts",
                            full_version)
    return parts, browser_channel, version_str

  @classmethod
  def _parse_channel(cls, matches) -> BrowserVersionChannel:
    channel_str: str = (matches["channel_long"] or matches["channel"] or
                        "stable").lower()
    return cls._CHANNEL_LOOKUP[channel_str]

  @classmethod
  def _validate_prefix(cls, prefix: Optional[str]) -> bool:
    if not prefix:
      return True
    return bool(cls._PREFIX_RE.match(prefix))

  def _channel_name(self, channel: BrowserVersionChannel) -> str:
    if channel == BrowserVersionChannel.LTS:
      return "esr"
    if channel == BrowserVersionChannel.STABLE:
      return "stable"
    if channel == BrowserVersionChannel.BETA:
      return "dev"
    if channel == BrowserVersionChannel.ALPHA:
      return "nightly"
    raise ValueError(f"Unsupported channel: {channel}")

  @property
  def has_complete_parts(self) -> bool:
    return len(self.parts) == 3

  @property
  def key(self) -> Tuple[Tuple[int, ...], BrowserVersionChannel]:
    return (self.comparable_parts(self._PARTS_LEN), self._channel)
