# Copyright 2023 The Chromium Authors
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

from __future__ import annotations

from typing import TYPE_CHECKING

from crossbench import plt
from crossbench.browsers.attributes import BrowserAttributes
from crossbench.browsers.chromium.chromium import Chromium

if TYPE_CHECKING:
  from crossbench.path import RemotePath


class EdgePathMixin:
  @classmethod
  def default_path(cls, platform: plt.Platform) -> RemotePath:
    return cls.stable_path(platform)

  @classmethod
  def stable_path(cls, platform: plt.Platform) -> RemotePath:
    return platform.search_app_or_executable(
        "Edge Stable",
        macos=["Microsoft Edge.app"],
        linux=["microsoft-edge"],
        win=["Microsoft/Edge/Application/msedge.exe"])

  @classmethod
  def beta_path(cls, platform: plt.Platform) -> RemotePath:
    return platform.search_app_or_executable(
        "Edge Beta",
        macos=["Microsoft Edge Beta.app"],
        linux=["microsoft-edge-beta"],
        win=["Microsoft/Edge Beta/Application/msedge.exe"])

  @classmethod
  def dev_path(cls, platform: plt.Platform) -> RemotePath:
    return platform.search_app_or_executable(
        "Edge Dev",
        macos=["Microsoft Edge Dev.app"],
        linux=["microsoft-edge-dev"],
        win=["Microsoft/Edge Dev/Application/msedge.exe"])

  @classmethod
  def canary_path(cls, platform: plt.Platform) -> RemotePath:
    return platform.search_app_or_executable(
        "Edge Canary",
        macos=["Microsoft Edge Canary.app"],
        linux=[],
        win=["Microsoft/Edge SxS/Application/msedge.exe"])

  @property
  def type_name(self) -> str:
    return "edge"


class Edge(EdgePathMixin, Chromium):
  DEFAULT_FLAGS = (
      "--enable-benchmarking",
      "--disable-extensions",
      "--no-first-run",
  )

  @property
  def attributes(self) -> BrowserAttributes:
    return BrowserAttributes.EDGE | BrowserAttributes.CHROMIUM_BASED
