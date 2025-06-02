# Copyright 2023 The Chromium Authors
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.
""" A collection of helpers that rely on non-crossbench code."""

from __future__ import annotations

import enum
import pathlib
import sys
import textwrap
from typing import List, Optional, Tuple, NamedTuple, cast

import tabulate

if sys.version_info >= (3, 11):
  from enum import StrEnum
else:

  class StrEnum(str, enum.Enum):

    def __str__(self) -> str:
      return str(self.value)


if sys.version_info >= (3, 9):

  def is_relative_to(path_a: pathlib.Path, path_b: pathlib.Path) -> bool:
    return path_a.is_relative_to(path_b)
else:

  def is_relative_to(path_a: pathlib.Path, path_b: pathlib.Path) -> bool:
    try:
      path_a.relative_to(path_b)
      return True
    except ValueError:
      return False


class StrHelpDataMixin(NamedTuple):
  value: str
  help: str


class StrEnumWithHelp(StrHelpDataMixin, enum.Enum):

  @classmethod
  def _missing_(cls, value) -> Optional[StrEnumWithHelp]:
    value = str(value)
    for member in cls:  # pytype: disable=missing-parameter
      if member.value == value:
        return member
    return None

  @classmethod
  def help_text_items(cls) -> List[Tuple[str, str]]:
    return [
        (repr(instance.value), instance.help) for instance in cls  # pytype: disable=missing-parameter
    ]

  @classmethod
  def help_text(cls, indent: int = 0) -> str:
    text: str = tabulate.tabulate(cls.help_text_items(), tablefmt="plain")
    if indent:
      return textwrap.indent(text, " " * indent)
    return text

  def __str__(self) -> str:
    return cast(str, self.value)
