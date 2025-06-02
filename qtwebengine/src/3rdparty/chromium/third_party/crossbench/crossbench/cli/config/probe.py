# Copyright 2023 The Chromium Authors
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

from __future__ import annotations
import argparse
import dataclasses
import re

from typing import TYPE_CHECKING, Any, Dict, Final, Iterable, List, Optional, TextIO, Type

import hjson
from crossbench import cli_helper, exception
from crossbench.config import ConfigObject

from crossbench.probes.all import GENERAL_PURPOSE_PROBES

if TYPE_CHECKING:
  from crossbench.probes.probe import Probe


class ProbeConfigError(argparse.ArgumentTypeError):
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
  def loads(cls, value: str) -> ProbeConfig:
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
    return cls.load_dict(config)

  @classmethod
  def load_dict(cls, config: Dict[str, Any]) -> ProbeConfig:
    probe_name = config.pop("name")
    if probe_name not in PROBE_LOOKUP:
      raise ProbeConfigError(f"Unknown probe: '{probe_name}'")
    probe_cls = PROBE_LOOKUP[probe_name]
    return cls(probe_cls, config)

  @property
  def name(self) -> str:
    return self.cls.NAME


# TODO: Migrate to ConfigObject
class ProbeListConfig:

  _PROBE_RE: Final[re.Pattern] = re.compile(
      r"(?P<probe_name>[\w.]+)(:?(?P<config>\{.*\}))?",
      re.MULTILINE | re.DOTALL)

  @classmethod
  def from_cli_args(cls, args: argparse.Namespace) -> ProbeListConfig:
    with exception.annotate_argparsing():
      if args.probe_config:
        with args.probe_config.open(encoding="utf-8") as f:
          return cls.load(f)
      return cls(args.probe)

  @classmethod
  def load(cls, file: TextIO) -> ProbeListConfig:
    # Make sure we wrap any exception in a argparse.ArgumentTypeError)
    with exception.annotate_argparsing():
      probe_config = cls()
      probe_config.load_config_file(file)
      return probe_config

  def __init__(self, probe_configs: Optional[Iterable[ProbeConfig]] = None):
    self._probes: List[Probe] = []
    if not probe_configs:
      return
    for probe_config in probe_configs:
      with exception.annotate(f"Parsing --probe={probe_config.name}"):
        self.add_probe(probe_config)

  @property
  def probes(self) -> List[Probe]:
    return self._probes

  def add_probe(self, probe_config: ProbeConfig) -> None:
    probe: Probe = probe_config.cls.from_config(probe_config.config)
    self._probes.append(probe)

  def load_config_file(self, file: TextIO) -> None:
    with exception.annotate(f"Loading probe config file: {file.name}"):
      data = None
      with exception.annotate(f"Parsing {hjson.__name__}"):
        try:
          data = hjson.load(file)
        except ValueError as e:
          raise ProbeConfigError(f"Parsing error: {e}") from e
      if not isinstance(data, dict) or "probes" not in data:
        raise ProbeConfigError(
            "Probe config file does not contain a 'probes' dict value.")
      self.load_dict(data["probes"])

  def load_dict(self, config: Dict[str, Any]) -> None:
    for probe_name, config_data in config.items():
      with exception.annotate(f"Parsing probe config probes['{probe_name}']"):
        if probe_name not in PROBE_LOOKUP:
          self.raise_unknown_probe(probe_name)
        probe_cls = PROBE_LOOKUP[probe_name]
        self._probes.append(probe_cls.from_config(config_data))

  def raise_unknown_probe(self, probe_name: str) -> None:
    additional_msg = ""
    if ":" in probe_name or "}" in probe_name:
      additional_msg = "\n    Likely missing quotes for --probe argument"
    msg = f"    Options are: {list(PROBE_LOOKUP.keys())}{additional_msg}"
    raise ProbeConfigError(f"Unknown probe name: '{probe_name}'\n{msg}")
