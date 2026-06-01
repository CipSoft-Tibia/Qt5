# Copyright 2024 The Chromium Authors
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

from __future__ import annotations

import argparse
from typing import (TYPE_CHECKING, Any, Dict, Iterable, List, Optional,
                    Sequence, Type)

from crossbench import exception
from crossbench.cli.config.probe import ProbeConfig, ProbeConfigError
from crossbench.config import ConfigObject
from crossbench.parse import ObjectParser

if TYPE_CHECKING:
  from crossbench.probes.probe import Probe


class ProbeListConfig(ConfigObject):

  @classmethod
  def from_cli_args(cls, args: argparse.Namespace) -> ProbeListConfig:
    with exception.annotate_argparsing():
      if args.probe_config:
        return cls.parse_path(args.probe_config)
      return cls(args.probe)

  @classmethod
  def parse_other(cls: Type[ProbeListConfig], value: Any) -> ProbeListConfig:
    if isinstance(value, (tuple, list)):
      return cls.parse_sequence(value)
    return super().parse_other(value)

  @classmethod
  def parse_sequence(cls: Type[ProbeListConfig],
                     config: Sequence[Dict[str, Any]]) -> ProbeListConfig:
    probe_configs: List[ProbeConfig] = []
    for index, probe_config in enumerate(config):
      probe_config = ObjectParser.dict(probe_config, f"probes[{index}]")
      probe_configs.append(ProbeConfig.parse_dict(probe_config))
    return cls(probe_configs)

  @classmethod
  def parse_dict(cls: Type[ProbeListConfig],
                 config: Dict[str, Any]) -> ProbeListConfig:
    # Support global configs with {"probes": ...}
    if "probes" in config:
      config = config["probes"]
      if isinstance(config, (tuple, list)):
        return cls.parse_sequence(config)
    elif "browsers" in config or "flags" in config:
      raise ProbeConfigError("Missing 'probes' property in global config.")
    config = ObjectParser.dict(config, "probes")
    probe_configs: List[ProbeConfig] = []
    for probe_name, config_data in config.items():
      with exception.annotate(f"Parsing probe config probes['{probe_name}']"):
        probe_configs.append(
            ProbeConfig.parse_probe_dict(probe_name, config_data))
    return cls(probe_configs)

  @classmethod
  def parse_str(cls, value: str) -> ProbeListConfig:
    raise NotImplementedError()

  def __init__(self, probes: Optional[Iterable[ProbeConfig]] = None):
    self._probes: List[Probe] = []
    if not probes:
      return
    for probe_config in probes:
      with exception.annotate(f"Parsing --probe={probe_config.name}"):
        self._add_probe(probe_config)

  @property
  def probes(self) -> List[Probe]:
    return self._probes

  def _add_probe(self, probe_config: ProbeConfig) -> None:
    probe: Probe = probe_config.cls.from_config(probe_config.config)
    self._probes.append(probe)
