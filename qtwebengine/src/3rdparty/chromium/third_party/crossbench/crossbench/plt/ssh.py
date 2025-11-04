# Copyright 2024 The Chromium Authors
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

from __future__ import annotations

import abc
from typing import TYPE_CHECKING

if TYPE_CHECKING:
  from crossbench.plt.base import CmdArg, ListCmdArgs


class SshPlatformMixin(abc.ABC):

  @property
  def is_remote_ssh(self) -> bool:
    return True

  @abc.abstractmethod
  def _build_ssh_cmd(self, *args: CmdArg, shell=False) -> ListCmdArgs:
    pass
