# Copyright 2023 The Chromium Authors
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

from __future__ import annotations

import argparse
import dataclasses
import re
from typing import (TYPE_CHECKING, Any, Dict, Final, Iterable, List, Optional,
                    Sequence, Type)

from crossbench import cli_helper, exception
from crossbench.config import ConfigError, ConfigObject
from crossbench.probes.all import GENERAL_PURPOSE_PROBES

if TYPE_CHECKING:
  from crossbench.probes.probe import Probe


class ProbeConfigError(ConfigError):
  pass


PROBE_LOOKUP: Dict[str, Type[Probe]] = {
    cls.NAME: cls for cls in GENERAL_PURPOSE_PROBES
}

_PROBE_CONFIG_RE: Final[re.Pattern] = re.compile(
    r"(?P<probe_name>[\w.]+)(:?(?P<config>\{.*\}))?", re.MULTILINE | re.DOTALL)


@dataclasses.dataclass(frozen=True)
class ProbeConfig(ConfigObject):
  cls: Type[Probe]
  config: Dict[str, Any] = dataclasses.field(default_factory=dict)

  def __post_init__(self) -> None:
    if not self.cls:
      raise ValueError(f"{type(self).__name__}.cls cannot be None.")
    if self.config is None:
      raise ValueError(f"{type(self).__name__}.config cannot be None.")

  @classmethod
  def parse_str(cls, value: str) -> ProbeConfig:
    # 1. variant: known probe
    if value in PROBE_LOOKUP:
      return cls(PROBE_LOOKUP[value])
    if cls.value_has_path_prefix(value):
      # ConfigObject.parse handles .hjson paths already, additional paths are
      # not supported in ProbeConfig.loads.
      raise ProbeConfigError(f"Probe config path does not exist: {value}")
    # 2. variant, inline hjson: "name:{hjson}"
    match = _PROBE_CONFIG_RE.fullmatch(value)
    if match is None:
      raise ProbeConfigError(f"Could not parse probe argument: {value}")
    config = {"name": match["probe_name"]}
    if config_str := match["config"]:
      inline_config = cli_helper.parse_inline_hjson(config_str)
      if "name" in inline_config:
        raise ProbeConfigError("Inline hjson cannot redefine 'name'.")
      config.update(inline_config)
    return cls.parse_dict(config)

  @classmethod
  def parse_dict(cls, config: Dict[str, Any]) -> ProbeConfig:
    probe_name = cli_helper.parse_non_empty_str(config.pop("name"), "name")
    return cls.parse_probe_dict(probe_name, config)

  @classmethod
  def parse_probe_dict(cls, probe_name: str, config: Dict[str,
                                                          Any]) -> ProbeConfig:
    if probe_cls := PROBE_LOOKUP.get(probe_name):
      return cls(probe_cls, config)
    raise cls._unknown_probe_error(probe_name)

  @classmethod
  def _unknown_probe_error(cls, probe_name: str) -> ProbeConfigError:
    additional_msg = ""
    if ":" in probe_name or "}" in probe_name:
      additional_msg = "\n    Likely missing quotes for --probe argument"
    msg = f"    Options are: {list(PROBE_LOOKUP.keys())}{additional_msg}"
    return ProbeConfigError(f"Unknown probe name: '{probe_name}'\n{msg}")

  @property
  def name(self) -> str:
    return self.cls.NAME


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
      probe_config = cli_helper.parse_dict(probe_config, f"probes[{index}]")
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
    config = cli_helper.parse_dict(config, "probes")
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
