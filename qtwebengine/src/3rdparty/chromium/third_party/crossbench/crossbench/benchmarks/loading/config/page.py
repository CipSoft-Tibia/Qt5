# Copyright 2024 The Chromium Authors
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

from __future__ import annotations

import dataclasses
import datetime as dt
from typing import Any, Dict, Iterator, Optional, Sequence, Tuple, Type, cast
from urllib import parse as urlparse

from crossbench import cli_helper
from crossbench import path as pth
from crossbench.benchmarks.loading.action import Action, GetAction
from crossbench.benchmarks.loading.action_type import ActionType
from crossbench.benchmarks.loading.config.blocks import (ActionBlock,
                                                         ActionBlockListConfig)
from crossbench.benchmarks.loading.config.login.custom import LoginBlock
from crossbench.benchmarks.loading.page import PAGES
from crossbench.benchmarks.loading.playback_controller import \
    PlaybackController
from crossbench.config import ConfigObject, ConfigParser


@dataclasses.dataclass(frozen=True)
class PageConfig(ConfigObject):
  label: Optional[str] = None
  playback: Optional[PlaybackController] = None
  blocks: Tuple[ActionBlock, ...] = tuple()
  login: Optional[LoginBlock] = None

  @classmethod
  def parse_other(cls: Type[PageConfig], value: Any, **kwargs) -> PageConfig:
    if isinstance(value, (list, tuple)):
      return cls.parse_sequence(value, **kwargs)
    return super().parse_other(value)

  @classmethod
  def parse_str(  # pylint: disable=arguments-differ
      cls: Type[PageConfig],
      value: str,
      label: Optional[str] = None) -> PageConfig:
    """
    Simple comma-separated string with optional duration:
      value = URL,[DURATION]
    """
    parts = value.rsplit(",", maxsplit=1)
    duration = dt.timedelta()
    raw_url: str = parts[0]
    if raw_url in PAGES:
      url = PAGES[raw_url].url
      label = label or raw_url
    else:
      url = cli_helper.parse_fuzzy_url_str(raw_url)
    if len(parts) == 2:
      duration = cli_helper.Duration.parse_non_zero(parts[1])
    return cls.from_url(label, url, duration)

  @classmethod
  def parse_sequence(cls: Type[PageConfig],
                     value: Sequence[Any],
                     label: Optional[str] = None) -> PageConfig:
    value = cli_helper.parse_non_empty_sequence(value,
                                                "story actions or blocks")
    blocks = ActionBlockListConfig.parse_sequence(value)
    if label is not None:
      label = cli_helper.parse_non_empty_str(label, "label")
    return cls(label, blocks=blocks.blocks)

  @classmethod
  def parse_dict(  # pylint: disable=arguments-differ
      cls: Type[PageConfig],
      config: Dict[str, Any],
      label: Optional[str] = None) -> PageConfig:
    config = cli_helper.parse_non_empty_dict(config, "story actions or blocks")
    page_config = cls.config_parser().parse(config, label=label)
    return page_config

  @classmethod
  def config_parser(cls: Type[PageConfig]) -> ConfigParser[PageConfig]:
    parser = ConfigParser(f"{cls.__name__} parser", cls)
    parser.add_argument("label", type=cli_helper.parse_non_empty_str)
    parser.add_argument("playback", type=PlaybackController.parse)
    parser.add_argument(
        "blocks",
        aliases=("actions", "url", "urls"),
        type=ActionBlockListConfig)
    parser.add_argument("login", type=LoginBlock.parse)
    return parser

  @classmethod
  def from_url(cls,
               label: Optional[str],
               url: str,
               duration: dt.timedelta = dt.timedelta()) -> PageConfig:
    actions = (GetAction(url, duration=duration),)
    blocks = (ActionBlock(actions=actions),)
    return PageConfig(label=label, blocks=blocks)

  def actions(self) -> Iterator[Action]:
    for block in self.blocks:
      yield from block

  @property
  def duration(self) -> dt.timedelta:
    return sum((action.duration for action in self.actions()), dt.timedelta())

  @property
  def any_label(self) -> str:
    return self.label or self.url_label

  @property
  def url_label(self) -> str:
    url = urlparse.urlparse(self.first_url)
    if url.scheme == "about":
      return url.path
    if url.scheme == "file":
      return pth.LocalPath(url.path).name
    if hostname := url.hostname:
      if hostname.startswith("www."):
        return hostname[len("www."):]
      return hostname
    return str(url)

  @property
  def first_url(self) -> str:
    for action in self.actions():
      if action.TYPE == ActionType.GET:
        return cast(GetAction, action).url
    raise RuntimeError("No GET action with an URL found.")