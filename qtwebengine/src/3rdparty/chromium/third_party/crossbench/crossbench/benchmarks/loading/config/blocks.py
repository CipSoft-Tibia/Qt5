# Copyright 2024 The Chromium Authors
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

from __future__ import annotations

import argparse
import dataclasses
import datetime as dt
from typing import (Any, Dict, Final, Iterator, List, Optional, Sequence, Tuple,
                    Type)

from crossbench import cli_helper, exception
from crossbench.benchmarks.loading.action import Action, ActionType
from crossbench.config import ConfigError, ConfigObject, ConfigParser


@dataclasses.dataclass(frozen=True)
class ActionBlock(ConfigObject):
  LOGIN_LABEL: Final[str] = "login"

  label: str = "default"
  index: int = 0
  actions: Tuple[Action, ...] = tuple()

  @classmethod
  def parse_str(cls: Type[ActionBlock], value: str) -> ActionBlock:
    raise NotImplementedError("Cannot create action blocks from strings")

  @classmethod
  def parse_other(cls: Type[ActionBlock], value: Any, **kwargs) -> ActionBlock:
    if isinstance(value, (tuple, list)):
      return cls.parse_sequence(value, **kwargs)
    return super().parse_other(value, **kwargs)

  @classmethod
  def parse_dict(  # pylint: disable=arguments-differ
      cls: Type,
      config: Dict[str, Any],
      label: Optional[str] = None,
      index: Optional[int] = None):
    return cls.config_parser().parse(config, label=label, index=index)

  @classmethod
  def config_parser(cls: Type[ActionBlock]) -> ConfigParser[ActionBlock]:
    parser = ConfigParser(f"{cls.__name__} parser", cls)
    parser.add_argument("label", type=cls._parse_block_label, default="default")
    parser.add_argument(
        "index",
        type=cli_helper.parse_positive_zero_int,
        default=0,
        required=False)
    # TODO: enable passing index
    parser.add_argument("actions", type=Action, required=True, is_list=True)
    return parser

  @classmethod
  def parse_sequence(cls: Type[ActionBlock],
                     config: Sequence[Dict[str, Any]],
                     label: Optional[str] = None,
                     index: Optional[int] = None) -> ActionBlock:
    with exception.annotate_argparsing(
        "Parsing default block action sequence:"):
      return cls.parse_dict({"actions": config}, label=label, index=index)

  @classmethod
  def _parse_block_label(cls, value: Any) -> Optional[str]:
    if not value:
      return None
    label = cli_helper.parse_non_empty_str(value)
    if label == cls.LOGIN_LABEL:
      raise ConfigError(
          f"Block label {repr(label)} is reserved for login blocks")
    return value

  def validate(self) -> None:
    super().validate()
    cli_helper.parse_non_empty_sequence(self.actions, "actions")
    # TODO: enable validating action indices
    # for index, action in enumerate(self.actions):
    #   if index != action.index:
    #     raise ValueError(
    #         f"action[{index}].index should be {index}, but got {action.index}")
    if not self.actions:
      raise argparse.ArgumentTypeError("Invalid block without actions")

  def to_json(self) -> Dict[str, Any]:
    return {
        "label": self.label,
        "actions": [action.to_json() for action in self.actions]
    }

  @property
  def duration(self) -> dt.timedelta:
    total_duration = dt.timedelta()
    for action in self.actions:
      if duration := action.duration:
        total_duration += duration
    return total_duration

  @property
  def is_login(self) -> bool:
    return False

  def __iter__(self) -> Iterator[Action]:
    yield from self.actions

  def __len__(self) -> int:
    return len(self.actions)


@dataclasses.dataclass(frozen=True)
class ActionBlockListConfig(ConfigObject):
  blocks: Tuple[ActionBlock, ...] = tuple()

  def to_argument_value(self) -> Tuple[ActionBlock, ...]:
    return self.blocks

  @classmethod
  def parse_other(cls: Type[ActionBlockListConfig],
                  value: Any) -> ActionBlockListConfig:
    if isinstance(value, (tuple, list)):
      return cls.parse_sequence(value)
    return super().parse_other(value)

  @classmethod
  def parse_sequence(cls: Type[ActionBlockListConfig],
                     config: Sequence[Dict[str, Any]]) -> ActionBlockListConfig:
    """Parse either a sequence of blocks or a sequence of actions for an
    implicit default block.

    Blocks:
    [{ "label": "block 1", "actions": [...]}, ... ]
    [ "block 1": [{ "action": ...}, ...], "block 2": [ ... ] ]

    Default block actions:
    [{ "action": "get", ...}, { "action": ...}, ...]
    """
    config = cli_helper.parse_non_empty_sequence(config, "actions")
    info = "action block"
    if cls._is_default_block_actions(config):
      info = "default actions"
      config = [{"actions": config}]
    if not cls._is_block_sequence_config(config):
      raise ValueError(
          "Invalid data: Expected a list of either blocks or actions.")

    def block_config_data_gen():
      for index, block_config in enumerate(config):
        with exception.annotate_argparsing(f"Parsing {info} ...[{index}]"):
          block_config = cli_helper.parse_dict(block_config, f"blocks[{index}]")
          label = block_config.get("label")
          yield index, label, block_config

    return cls._parse_blocks(block_config_data_gen())

  @classmethod
  def _is_block_sequence_config(cls, config: Sequence[Dict[str, Any]]) -> bool:
    return "label" in config[0] or "actions" in config[0]

  @classmethod
  def _is_default_block_actions(cls, config: Sequence[Dict[str, Any]]) -> bool:
    sample = config[0]
    return isinstance(sample, str) or "action" in sample

  @classmethod
  def parse_dict(cls: Type[ActionBlockListConfig],
                 config: Dict[str, Any]) -> ActionBlockListConfig:
    config = cli_helper.parse_non_empty_dict(config, "blocks")

    def block_config_data_gen():
      for index, (label, block_data) in enumerate(config.items()):
        with exception.annotate_argparsing(
            f"Parsing action block  ...[{label}]"):
          yield index, label, block_data

    return cls._parse_blocks(block_config_data_gen())

  @classmethod
  def _parse_blocks(cls, block_config_data_gen) -> ActionBlockListConfig:
    blocks: List[ActionBlock] = []
    for index, label, block_data in block_config_data_gen:
      block = cls._parse_block(index, label, block_data)
      blocks.append(block)
    return cls(tuple(blocks))

  @classmethod
  def _parse_block(cls, index: int, label: str, block_data: Any) -> ActionBlock:
    if isinstance(block_data, dict):
      # Early warning for better usability.
      if inner_label := block_data.get("label"):
        if inner_label != label:
          raise ConfigError(
              "ActionBlock inside a dict cannot have a 'label' property, "
              f"but got label={repr(inner_label)}")
    return ActionBlock.parse(block_data, label=label, index=index)

  @classmethod
  def parse_str(cls, value: str) -> ActionBlockListConfig:
    raise NotImplementedError("Cannot create action blocks from strings")

  def validate(self) -> None:
    super().validate()
    if not self.blocks:
      raise ValueError("Missing action blocks.")
    cli_helper.parse_non_empty_sequence(self.blocks, "blocks")
    found_get = False
    for index, block in enumerate(self.blocks):
      if index != block.index:
        raise ValueError(
            f"blocks[{index}].index should be {index}, but got {block.index}")
      found_get |= any(action.TYPE == ActionType.GET for action in block)
    if not found_get:
      raise ValueError("Expected at least one get action in one of the blocks.")
