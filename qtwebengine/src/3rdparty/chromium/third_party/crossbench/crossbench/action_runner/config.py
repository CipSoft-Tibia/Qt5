# Copyright 2024 The Chromium Authors
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

from __future__ import annotations

import argparse
from typing import Any

from crossbench.action_runner.android_input_action_runner import \
    AndroidInputActionRunner
from crossbench.action_runner.base import ActionRunner
from crossbench.action_runner.chromeos_input_action_runner import \
    ChromeOSInputActionRunner
from crossbench.action_runner.default_action_runner import DefaultActionRunner


# TODO: migrate to full config.ConfigObject
class ActionRunnerConfig:

  @classmethod
  def parse(cls, value: Any) -> ActionRunner:
    if isinstance(value, ActionRunner):
      return value
    if value == "basic":
      return DefaultActionRunner()
    if value == "android":
      return AndroidInputActionRunner()
    if value == "chromeos":
      return ChromeOSInputActionRunner()
    raise argparse.ArgumentTypeError(
      f"Invalid choice '{value}', allowed values are 'basic', 'android', "
      "'chromeos'"
    )
