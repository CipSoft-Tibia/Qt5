# Copyright 2024 The Chromium Authors
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

from __future__ import annotations

from typing import TYPE_CHECKING

if TYPE_CHECKING:
  from crossbench.path import LocalPath, RemotePathLike
  from crossbench.plt.base import Platform


class RemotePlatformMixin:

  def __init__(self, host_platform: Platform):
    super().__init__()
    self._host_platform: Platform = host_platform

  @property
  def is_remote(self) -> bool:
    return True

  @property
  def host_platform(self) -> Platform:
    return self._host_platform

  def host_path(self, path: RemotePathLike) -> LocalPath:
    return self._host_platform.local_path(path)
