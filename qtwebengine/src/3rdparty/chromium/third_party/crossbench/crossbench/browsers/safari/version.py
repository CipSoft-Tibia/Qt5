# Copyright 2023 The Chromium Authors
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

from __future__ import annotations

import re
from typing import Final, Tuple

from crossbench.browsers.version import BrowserVersion, BrowserVersionChannel


class SafariVersion(BrowserVersion):
  _MIN_PARTS_LEN: Final[int] = 4
  _VERSION_RE = re.compile(
      r"(?P<major_minor>\d+\.\d+)"
      r"[^(]+ "
      r"\((?P<version>(Release (?P<release>\d+), )?(?P<parts>([\d.]+)+))\)"
      r".*", re.I)

  @classmethod
  def _parse(
      cls,
      full_version: str) -> Tuple[Tuple[int, ...], BrowserVersionChannel, str]:
    matches = cls._VERSION_RE.fullmatch(full_version.strip())
    if not matches:
      raise cls.parse_error("Could not extract version number", full_version)
    version_str = matches["version"]
    parts_str = matches["parts"]
    major_minor_str = matches["major_minor"]
    assert version_str and parts_str and major_minor_str
    channel = cls._parse_channel(full_version)
    major, minor = tuple(map(int, major_minor_str.split(".")))
    release = 0
    if release_str := matches["release"]:
      release = int(release_str)
    try:
      parts = tuple(map(int, parts_str.split(".")))
    except ValueError as e:
      raise cls.parse_error("Could not parse version number parts.",
                            full_version) from e
    if len(parts) < cls._MIN_PARTS_LEN:
      raise cls.parse_error("Invalid number of version number parts",
                            full_version)
    parts = (major, minor, release) + parts
    return parts, channel, f"{major_minor_str} ({version_str})"

  @classmethod
  def _parse_channel(cls, full_version: str) -> BrowserVersionChannel:
    if "Safari Technology Preview" in full_version:
      return BrowserVersionChannel.BETA
    if " any" in full_version.lower():
      return BrowserVersionChannel.ANY
    return BrowserVersionChannel.STABLE

  @property
  def has_complete_parts(self) -> bool:
    return len(self.parts) >= self._MIN_PARTS_LEN

  @property
  def is_tech_preview(self) -> bool:
    return self.channel == BrowserVersionChannel.BETA

  @property
  def release(self) -> int:
    return self._parts[2]

  def _channel_name(self, channel: BrowserVersionChannel) -> str:
    if channel == BrowserVersionChannel.STABLE:
      return "stable"
    if channel == BrowserVersionChannel.BETA:
      return "technology preview"
    raise ValueError(f"Unsupported channel: {channel}")

  @property
  def key(self) -> Tuple[Tuple[int, ...], BrowserVersionChannel]:
    return (self.comparable_parts(self._MIN_PARTS_LEN), self._channel)
