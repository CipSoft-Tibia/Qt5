# Copyright 2024 The Chromium Authors
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

from __future__ import annotations

import logging
from typing import TYPE_CHECKING

from crossbench import path as pth
from crossbench import plt
from crossbench.plt.linux_ssh import LinuxSshPlatform

if TYPE_CHECKING:
  from typing import Optional

  from crossbench.flags.chrome import ChromeFlags


class ChromeOsSshPlatform(LinuxSshPlatform):

  AUTOLOGIN_PATH = pth.RemotePath("/usr/local/autotest/bin/autologin.py")
  DEVTOOLSPORT_PATH = pth.RemotePath("/home/chronos/DevToolsActivePort")

  @property
  def name(self) -> str:
    return "chromeos_ssh"

  def create_debugging_session(self,
                               browser_flags: Optional[ChromeFlags] = None
                              ) -> int:
    logging.info("Attempting autologin into a test session.")
    try:
      if browser_flags:
        self.sh(self.AUTOLOGIN_PATH, "--", *browser_flags)
      else:
        self.sh(self.AUTOLOGIN_PATH)
    except plt.SubprocessError as e:
      raise RuntimeError("Autologin failed.") from e
    try:
      dbg_port = self.cat(self.DEVTOOLSPORT_PATH).splitlines()[0].strip()
    except plt.SubprocessError as e:
      raise RuntimeError("Could not read remote debugging port.") from e
    return int(dbg_port)
