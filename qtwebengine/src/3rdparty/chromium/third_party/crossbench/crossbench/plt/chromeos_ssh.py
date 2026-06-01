# Copyright 2024 The Chromium Authors
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

from __future__ import annotations

import json
import logging
import subprocess
from typing import TYPE_CHECKING

from crossbench import path as pth
from crossbench import plt
from crossbench.parse import NumberParser, ObjectParser
from crossbench.plt.linux_ssh import LinuxSshPlatform

if TYPE_CHECKING:
  from typing import Optional, Tuple

  from crossbench.flags.chrome import ChromeFlags
  from crossbench.plt.base import ListCmdArgs


class ChromeOsSshPlatform(LinuxSshPlatform):

  AUTOLOGIN_PATH = pth.AnyPosixPath("/usr/local/autotest/bin/autologin.py")
  DEVTOOLS_PORT_PATH = pth.AnyPosixPath("/home/chronos/DevToolsActivePort")

  def __init__(self, *args, **kwargs):
    super().__init__(*args, **kwargs)
    self._username: Optional[str] = None
    # `/tmp` on ChromeOS is mounted with `noexec` flag.
    # Instead, we use `/usr/local/tmp`, which allows executions of binaries.
    self._default_tmp_dir = pth.AnyPosixPath("/usr/local/tmp")

  @property
  def name(self) -> str:
    return "chromeos_ssh"

  @property
  def username(self) -> Optional[str]:
    return self._username

  @property
  def is_chromeos(self) -> bool:
    return True

  def create_debugging_session(self,
                               browser_flags: Optional[Tuple[str, ...]] = None,
                               username: Optional[str] = None,
                               password: Optional[str] = None) -> int:
    try:
      args: ListCmdArgs = [self.AUTOLOGIN_PATH]
      if username and password:
        self._username = username
        args.extend(("-u", username, "-p", password))
      if browser_flags:
        args.append("--")
        args.extend(browser_flags)
      autologin_output = self.sh(
          *args, stdout=subprocess.PIPE,
          stderr=subprocess.STDOUT).stdout.decode("utf-8")
      logging.debug("Autologin Output:")
      logging.debug(autologin_output)
    except plt.SubprocessError as e:
      raise RuntimeError("Autologin failed.") from e
    try:
      dbg_port = self.cat(self.DEVTOOLS_PORT_PATH).splitlines()[0].strip()
    except plt.SubprocessError as e:
      raise RuntimeError("Could not read remote debugging port.") from e
    return int(dbg_port)

  def screenshot(self, result_path: pth.AnyPath) -> None:
    self.sh("screenshot", result_path)

  def display_resolution(self) -> Tuple[int, int]:
    display_info_json = self.sh_stdout("cros-health-tool", "telem",
                                       "--category=display")
    display_info = json.loads(display_info_json)
    display_info = ObjectParser.dict(display_info, "display info")
    embedded_display = ObjectParser.dict(display_info.get("embedded_display"))
    resolution_horizontal = NumberParser.positive_int(
        embedded_display.get("resolution_horizontal"), "resolution_horizontal")
    resolution_vertical = NumberParser.positive_int(
        embedded_display.get("resolution_vertical"), "resolution_vertical")
    return (resolution_horizontal, resolution_vertical)
