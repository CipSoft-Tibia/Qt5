# Copyright 2023 The Chromium Authors
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

from __future__ import annotations
import argparse
import pathlib

import hjson
from crossbench import cli_helper

from crossbench.env import HostEnvironment, HostEnvironmentConfig


def parse_inline_env_config(value: str) -> HostEnvironmentConfig:
  if value in HostEnvironment.CONFIGS:
    return HostEnvironment.CONFIGS[value]
  if value[0] != "{":
    raise argparse.ArgumentTypeError(
        f"Invalid env config name: '{value}'. "
        f"choices = {list(HostEnvironment.CONFIGS.keys())}")
  # Assume hjson data
  kwargs = None
  msg = ""
  try:
    kwargs = cli_helper.parse_inline_hjson(value)
    return HostEnvironmentConfig(**kwargs)
  except Exception as e:
    msg = f"\n{e}"
    raise argparse.ArgumentTypeError(
        f"Invalid inline config string: {value}{msg}") from e


def parse_env_config_file(value: str) -> HostEnvironmentConfig:
  config_path: pathlib.Path = cli_helper.parse_file_path(value)
  try:
    with config_path.open(encoding="utf-8") as f:
      data = hjson.load(f)
    if "env" not in data:
      raise argparse.ArgumentTypeError("No 'env' property found")
    kwargs = data["env"]
    return HostEnvironmentConfig(**kwargs)
  except Exception as e:
    msg = f"\n{e}"
    raise argparse.ArgumentTypeError(
        f"Invalid env config file: {value}{msg}") from e
