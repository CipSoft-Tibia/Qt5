# Copyright 2024 The Chromium Authors
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

from __future__ import annotations

import dataclasses
import functools
import logging
from typing import Any, Dict, Final, List, Optional, Sequence, Set, Tuple

from immutabledict import immutabledict
from ordered_set import OrderedSet

from crossbench import exception
from crossbench.browsers.browser_helper import convert_flags_to_label
from crossbench.config import ConfigError, ConfigObject
from crossbench.flags.base import Flags
from crossbench.parse import ObjectParser

DEFAULT_LABEL: Final[str] = "default"


@dataclasses.dataclass(frozen=True)
class FlagsVariantConfig:
  label: str
  index: int = 0
  flags: Flags = dataclasses.field(default_factory=lambda: Flags().freeze())

  @classmethod
  def parse(cls, name: str, index: int, data: Any) -> FlagsVariantConfig:
    return cls(name, index, Flags.parse(data).freeze())

  def merge_copy(self,
                 other: FlagsVariantConfig,
                 label: Optional[str] = None,
                 index: int = -1) -> FlagsVariantConfig:
    index = self.index if index < 0 else index
    new_label = label or f"{self.label}_{other.label}"
    return FlagsVariantConfig(new_label, index,
                              self.flags.merge_copy(other.flags).freeze())

  def __hash__(self) -> int:
    return hash(self.flags)

  def __eq__(self, other: Any) -> bool:
    if not isinstance(other, FlagsVariantConfig):
      return False
    return self.flags == other.flags


class FlagsGroupConfig(Tuple[FlagsVariantConfig, ...]):
  """
  Config container for a list of FlagsVariantConfig:
  FlagsGroupConfig(
    FlagsVariantConfig("default"),
    FlagsVariantConfig("max_opt_1", "--js-flags='--max-opt=1'),
    FlagsVariantConfig("max_opt_2", "--js-flags='--max-opt=2'),
    ...
  )
  """

  @classmethod
  def parse(cls, data: Any) -> FlagsGroupConfig:
    if data is None:
      return FlagsGroupConfig()
    if isinstance(data, str):
      return cls.parse_str(data)
    if isinstance(data, dict):
      return cls.parse_dict(data)
    if isinstance(data, (list, tuple)):
      return cls.parse_sequence(data)
    raise ConfigError(f"Invalid type {type(data)}: {repr(data)}")

  @classmethod
  def parse_dict(cls, config: Dict) -> FlagsGroupConfig:
    if not config:
      return FlagsGroupConfig()
    all_flag_keys = all(key.startswith("-") for key in config.keys())
    all_str_values = all(isinstance(value, str) for value in config.values())
    if not all_flag_keys:
      return cls.parse_dict_with_labels(config)
    if all_str_values:
      return cls.parse_dict_simple(config)
    return cls._parse_variants_dict(config)

  @classmethod
  def parse_dict_with_labels(cls, config: Dict) -> FlagsGroupConfig:
    variants: OrderedSet[FlagsVariantConfig] = OrderedSet()
    logging.debug("Using custom flag group labels")
    for label, value in config.items():
      with exception.annotate_argparsing(
          f"Parsing flag variant ...[{repr(label)}]:"):
        variant = FlagsVariantConfig.parse(label, len(variants), value)
        if variant in variants:
          raise ConfigError(f"Duplicate flag variant: {value}")
        variants.add(variant)
    return FlagsGroupConfig(tuple(variants))

  @classmethod
  def parse_dict_simple(cls, config: Dict) -> FlagsGroupConfig:
    logging.debug("Using single flag group dict")
    variants = (FlagsVariantConfig.parse(DEFAULT_LABEL, 0, config),)
    return FlagsGroupConfig(variants)

  @classmethod
  def _parse_variants_dict(cls, data: Dict[str, Any]) -> FlagsGroupConfig:
    # data == {
    #  "--flag": None,
    #  "--flag-b": "custom flag value",
    #  "--flag-c": (None, "value 2", "value 3"),
    # }
    cls._validate_variants_dict(data)
    per_flag_groups: List[FlagsGroupConfig] = []
    for flag_name, flag_data in data.items():
      per_flag_groups.append(cls._dict_variant_to_group(flag_name, flag_data))

    variants = per_flag_groups[0]
    for next_variant in per_flag_groups[1:]:
      variants = variants.product(next_variant)
    return variants

  @classmethod
  def _validate_variants_dict(cls, data: Dict[str, Any]) -> None:
    flags = Flags()
    for flag_name, flag_value in data.items():
      with exception.annotate_argparsing(
          f"Parsing flag variant ...[{flag_name}]:"):
        flags.set(flag_name)
        if flag_value is None:
          continue
        if not isinstance(flag_value, (str, list, tuple)):
          raise ConfigError(
              f"Invalid flag variant value (None, str or sequence): "
              f"{flag_name}={repr(flag_value)}")
        if isinstance(flag_value, (list, tuple)):
          ObjectParser.unique_sequence(
              flag_value, f"flag {repr(flag_name)} variant values", ConfigError)

  @classmethod
  def _dict_variant_to_group(cls, flag_name: str,
                             data: Any) -> FlagsGroupConfig:
    if data is None:
      return cls.parse_str(flag_name)
    if isinstance(data, str):
      data_str: str = data.strip()
      if not data_str:
        return cls.parse_str(flag_name)
      data = (data_str,)
    assert isinstance(data, (list, tuple)), "Invalid flag variant type"
    flags: OrderedSet[Optional[Flags]] = OrderedSet()
    for variant in data:
      if variant is None:
        flag = None
      elif not variant.strip():
        flag = Flags((flag_name,))
      else:
        cls._validate_variant_flag(flag_name, variant)
        flag = Flags({flag_name: variant})
      if flag in flags:
        raise ConfigError("Same flag variant was specified more than once: "
                          f"{repr(flag)} for entry {repr(flag_name)}")
      flags.add(flag)
    return cls.parse_sequence(flags)

  @classmethod
  def _validate_variant_flag(cls, flag_name: str, flag_value: Any) -> None:
    if flag_value == "None,":
      raise ConfigError("Please use null (from json) instead of "
                        f"None (from python) for flag {repr(flag_name)}")

  @classmethod
  def parse_sequence(cls, data: Sequence) -> FlagsGroupConfig:
    variants: List[FlagsVariantConfig] = []
    duplicates: Set[str] = set()
    for flag_data in data:
      if not flag_data:
        flags = Flags()
      else:
        flags = Flags.parse(flag_data)
      if flag_data in duplicates:
        raise ConfigError(f"Duplicate variant: {flags}")
      duplicates.add(flag_data)
      variants.append(
          FlagsVariantConfig(
              convert_flags_to_label(*flags), len(variants), flags))
    return FlagsGroupConfig(tuple(variants))

  @classmethod
  def parse_str(cls, value: str) -> FlagsGroupConfig:
    if not value.strip():
      return FlagsGroupConfig()
    variants = (FlagsVariantConfig.parse(DEFAULT_LABEL, 0, value),)
    return FlagsGroupConfig(variants)

  def product(self, *args: FlagsGroupConfig) -> FlagsGroupConfig:
    return functools.reduce(lambda a, b: a.inner_product(b), args, self)

  def inner_product(self, other: FlagsGroupConfig) -> FlagsGroupConfig:
    """Create a new FlagsGroupConfig as the combination of
    self.variants x other.variants"""
    new_variants: List[FlagsVariantConfig] = []
    new_labels: Set[str] = set()
    if not other:
      return self
    if not self:
      return other
    for variant in self:
      for variant_other in other:
        new_label = self._unique_product_label(new_labels, variant,
                                               variant_other)
        new_labels.add(new_label)
        new_variant: FlagsVariantConfig = variant.merge_copy(
            variant_other, index=len(new_variants), label=new_label)
        new_variants.append(new_variant)

    return FlagsGroupConfig(tuple(new_variants))

  def _unique_product_label(self, label_set: Set[str],
                            variant_a: FlagsVariantConfig,
                            variant_b: FlagsVariantConfig) -> str:
    default = f"{variant_a.label}_{variant_b.label}"
    if variant_a.label == DEFAULT_LABEL:
      default = variant_b.label
    if variant_b.label == DEFAULT_LABEL:
      default = variant_a.label
    label = default
    if not variant_a.flags:
      label = variant_b.label
    if not variant_b.flags:
      label = variant_a.label
    if label not in label_set:
      return label
    if default not in label_set:
      return default
    return f"{default}_{len(label_set)}"


class FlagsConfig(ConfigObject, immutabledict[str, FlagsGroupConfig]):

  @classmethod
  def parse_str(cls, value: str) -> FlagsConfig:
    if not value:
      raise ConfigError("Cannot parse empty string")
    return cls({"default": FlagsGroupConfig.parse_str(value)})

  @classmethod
  def parse_dict(cls, config: Dict[str, Any]) -> FlagsConfig:
    groups: Dict[str, FlagsGroupConfig] = {}
    for group_name, group_data in config.items():
      with exception.annotate(f"Parsing flag-group: flags[{repr(group_name)}]"):
        groups[group_name] = FlagsGroupConfig.parse(group_data)
    return cls(groups)
