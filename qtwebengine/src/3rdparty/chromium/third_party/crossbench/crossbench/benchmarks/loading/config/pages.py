# Copyright 2024 The Chromium Authors
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

from __future__ import annotations

import argparse
import dataclasses
import datetime as dt
import logging
from typing import Any, Dict, List, Optional, Sequence, Tuple, Type

from crossbench import cli_helper, exception
from crossbench import path as pth
from crossbench.benchmarks.loading.action import (Action, ClickAction,
                                                  GetAction, ReadyState,
                                                  WaitAction)
from crossbench.benchmarks.loading.config.blocks import ActionBlock
from crossbench.benchmarks.loading.config.page import PageConfig
from crossbench.benchmarks.loading.input_source import InputSource
from crossbench.browsers.secrets import SecretType
from crossbench.config import ConfigObject


@dataclasses.dataclass(frozen=True)
class PagesConfig(ConfigObject):
  pages: Tuple[PageConfig, ...] = ()
  logins: Tuple[SecretType, ...] = ()

  def validate(self) -> None:
    super().validate()
    for index, page in enumerate(self.pages):
      assert isinstance(page, PageConfig), (
          f"pages[{index}] is not a PageConfig but {type(page).__name__}")

  @classmethod
  def parse_str(cls, value: str) -> PagesConfig:
    """
    Simple comma-separate config:
    value = URL, [DURATION], ...
    """
    values: List[str] = []
    previous_part: Optional[str] = None
    for part in value.strip().split(","):
      part = cli_helper.parse_non_empty_str(part, "url or duration")
      try:
        cli_helper.Duration.parse_non_zero(part)
        if not previous_part:
          raise argparse.ArgumentTypeError(
              "Duration can only follow after url. "
              f"Current value: {repr(part)}")
        values[-1] = f"{previous_part},{part}"
        previous_part = None
      except cli_helper.DurationParseError:
        previous_part = part
        values.append(part)
    return cls.parse_sequence(values)

  @classmethod
  def parse_unknown_path(cls, path: pth.LocalPath, **kwargs) -> PagesConfig:
    # Make sure we get errors for invalid files.
    return cls.parse_config_path(path, **kwargs)

  @classmethod
  def parse_other(cls, value: Any, **kwargs) -> PagesConfig:
    if isinstance(value, (list, tuple)):
      return cls.parse_sequence(value, **kwargs)
    return super().parse_other(value, **kwargs)

  @classmethod
  def parse_sequence(cls, values: Sequence[str]) -> PagesConfig:
    """
    Variant a): List of comma-separate URLs
      [ "URL,[DURATION]", ... ]
    """
    # TODO: support parsing a list of PageConfig dicts
    if not values:
      raise argparse.ArgumentTypeError("Got empty page list.")
    pages: List[PageConfig] = []
    for index, single_line_config in enumerate(values):
      with exception.annotate_argparsing(
          f"Parsing pages[{index}]: {repr(single_line_config)}"):
        pages.append(PageConfig.parse_str(single_line_config))
    return PagesConfig(pages=tuple(pages))

  @classmethod
  def parse_dict(cls, config: Dict) -> PagesConfig:
    """
    Variant a):
    { "pages": { "LABEL": PAGE_CONFIG }}
    """
    with exception.annotate_argparsing("Parsing stories"):
      if "pages" not in config:
        raise argparse.ArgumentTypeError(
            "Config does not provide a 'pages' dict.")
      pages = cli_helper.parse_non_empty_dict(config["pages"], "pages")
      with exception.annotate_argparsing("Parsing config 'pages'"):
        logins = [SecretType.parse(login) for login in config.get("logins", [])]
        pages = cls._parse_pages(pages)
        return PagesConfig(pages, tuple(logins))
    raise exception.UnreachableError()

  @classmethod
  def _parse_pages(cls, data: Dict[str, Any]) -> Tuple[PageConfig, ...]:
    pages = []
    for name, page_config in data.items():
      with exception.annotate_argparsing(f"Parsing story ...['{name}']"):
        page = PageConfig.parse(page_config, label=name)
        pages.append(page)
    return tuple(pages)


class DevToolsRecorderPagesConfig(PagesConfig):

  @classmethod
  def parse_str(cls: Type[PagesConfig], value: str) -> PagesConfig:
    raise NotImplementedError()

  @classmethod
  def parse_dict(cls, config: Dict[str, Any]) -> DevToolsRecorderPagesConfig:
    config = cli_helper.parse_non_empty_dict(config)
    with exception.annotate_argparsing("Loading DevTools recording file"):
      title = cli_helper.parse_non_empty_str(config["title"], "title")
      actions = cls._parse_steps(config["steps"])
      # Use default block
      blocks = (ActionBlock(actions=actions),)
      pages = (PageConfig(label=title, blocks=blocks),)
      return DevToolsRecorderPagesConfig(pages)
    raise exception.UnreachableError()

  @classmethod
  def _parse_steps(cls, steps: List[Dict[str, Any]]) -> Tuple[Action, ...]:
    actions: List[Action] = []
    for step in steps:
      maybe_actions: Optional[Action] = cls._parse_step(step)
      if maybe_actions:
        actions.append(maybe_actions)
        # TODO(cbruni): make this configurable
        actions.append(WaitAction(duration=dt.timedelta(seconds=1)))
    return tuple(actions)

  @classmethod
  def _parse_step(cls, step: Dict[str, Any]) -> Optional[Action]:
    step_type: str = step["type"]
    default_timeout = dt.timedelta(seconds=10)
    if step_type == "navigate":
      return GetAction(  # type: ignore
          step["url"], ready_state=ReadyState.COMPLETE)
    if step_type == "click":
      selectors: List[List[str]] = step["selectors"]
      xpath: Optional[str] = None
      for selector_list in selectors:
        for selector in selector_list:
          if selector.startswith("xpath//"):
            xpath = selector
            break
      assert xpath, "Need xpath selector for click action"
      return ClickAction(
          InputSource.JS,
          selector=xpath,
          scroll_into_view=True,
          timeout=default_timeout)
    if step_type == "setViewport":
      # Resizing is ignored for now.
      return None
    raise ValueError(f"Unsupported step: {step_type}")


class ListPagesConfig(PagesConfig):

  VALID_EXTENSIONS: Tuple[str, ...] = (".txt", ".list")

  @classmethod
  def parse_str(cls, value: str) -> PagesConfig:
    raise argparse.ArgumentTypeError(
        f"URL list file {repr(value)} does not exist.")

  @classmethod
  def parse_path(cls, path: pth.LocalPath, **kwargs) -> PagesConfig:
    assert not kwargs, f"{cls.__name__} does not support extra kwargs"
    pages: List[PageConfig] = []
    with exception.annotate_argparsing(f"Loading Pages list file: {path.name}"):
      line: int = 0
      with path.open() as f:
        for single_line_config in f.readlines():
          with exception.annotate_argparsing(f"Parsing line {line}"):
            line += 1
            single_line_config = single_line_config.strip()
            if not single_line_config:
              logging.warning("Skipping empty line %s", line)
              continue
            pages.append(PageConfig.parse(single_line_config))
    return PagesConfig(pages=tuple(pages))

  @classmethod
  def parse_dict(cls, config: Dict) -> PagesConfig:
    config = cli_helper.parse_non_empty_dict(config, "pages")
    with exception.annotate_argparsing("Parsing scenarios / pages"):
      if "pages" not in config:
        raise argparse.ArgumentTypeError(
            "Config does not provide a 'pages' dict.")
      pages = config["pages"]
      if isinstance(pages, str):
        pages = [pages]
      if not isinstance(pages, (list, tuple)):
        raise argparse.ArgumentTypeError(
            f"Expected list/tuple for pages, but got {type(pages)}")
      return cls.parse_sequence(pages)
    raise exception.UnreachableError()
