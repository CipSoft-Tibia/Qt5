# Copyright 2024 The Chromium Authors
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

from __future__ import annotations

import re
from dataclasses import dataclass
from typing import TYPE_CHECKING, Dict

if TYPE_CHECKING:
  from crossbench.plt.base import Platform

pattern = re.compile(
    r"(?P<name>[^\(\)]) \((?P<version>[0-9\.]+)\) \((?P<uuid>[0-9A-Z-]+)\)")


@dataclass(frozen=True)
class IOSDevice:
  name: str
  version: str
  uuid: str

  def __str__(self) -> str:
    return f"{self.name} ({self.version}) ({self.uuid})"


def ios_devices(platform: Platform,
                show_all: bool = False) -> Dict[str, IOSDevice]:
  output = platform.sh_stdout("xcrun", "xctrace", "list", "devices")
  category_index = 0
  results: Dict[str, IOSDevice] = {}
  for line in output.splitlines():
    if line.startswith("== "):
      category_index += 1
      continue
    if category_index > 1 and not show_all:
      return results

    for match in pattern.finditer(line):
      device = IOSDevice(
          match.group("name"), match.group("version"), match.group("uuid"))
      if device.uuid in results:
        raise ValueError("Invalid UUID")
      results[device.uuid] = device
  return results
