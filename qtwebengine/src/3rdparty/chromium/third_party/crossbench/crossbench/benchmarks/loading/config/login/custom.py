# Copyright 2024 The Chromium Authors
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

from __future__ import annotations

import dataclasses
from typing import Final

from crossbench.benchmarks.loading.config.blocks import ActionBlock


@dataclasses.dataclass(frozen=True)
class LoginBlock(ActionBlock):
  LABEL: Final[str] = "login"

  def validate(self):
    super().validate()
    assert self.index == 0, (
        f"Login block has to be the first, but got {self.index}")

  @property
  def is_login(self) -> bool:
    return True
