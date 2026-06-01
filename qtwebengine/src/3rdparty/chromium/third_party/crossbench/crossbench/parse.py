# Copyright 2023 The Chromium Authors
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

from __future__ import annotations

import argparse
import datetime as dt
import enum
import json
import logging
import math
import re
import shlex
from typing import (Any, Dict, Final, Iterable, List, Optional, Sequence, Type,
                    TypeVar, Union, cast)
from urllib import parse as urlparse

import hjson

from crossbench import path as pth
from crossbench import plt


def type_str(value: Any) -> str:
  return type(value).__name__


class PathParser:

  PATH_PREFIX = re.compile(r"^(?:"
                           r"(?:\.\.?|~)?|"
                           r"[a-zA-Z]:"
                           r")(\\|/)[^\\/]")

  @classmethod
  def path(cls,
           value: Optional[pth.AnyPathLike],
           name: str = "value") -> pth.LocalPath:
    path_value: pth.AnyPathLike = ObjectParser.not_none(value,
                                                        "path")  # type: ignore
    if not path_value:
      raise argparse.ArgumentTypeError("Invalid empty path.")
    try:
      path = pth.LocalPath(path_value).expanduser()
    except RuntimeError as e:
      raise argparse.ArgumentTypeError(
          f"Invalid Path {name} {repr(value)}': {e}") from e
    return path

  @classmethod
  def existing_file_path(cls,
                         value: pth.AnyPathLike,
                         name: str = "value") -> pth.LocalPath:
    path = cls.existing_path(value, name)
    if not path.is_file():
      raise argparse.ArgumentTypeError(
          f"{name} is not a file: {repr(str(path))}")
    return path

  @classmethod
  def non_empty_file_path(cls,
                          value: pth.AnyPathLike,
                          name: str = "value") -> pth.LocalPath:
    path: pth.LocalPath = cls.existing_file_path(value, name)
    if path.stat().st_size == 0:
      raise argparse.ArgumentTypeError(
          f"{name} is an empty file: {repr(str(path))}")
    return path

  @classmethod
  def file_path(cls,
                value: pth.AnyPathLike,
                name: str = "value") -> pth.LocalPath:
    return cls.non_empty_file_path(value, name)

  @classmethod
  def dir_path(cls,
               value: pth.AnyPathLike,
               name: str = "value") -> pth.LocalPath:
    path = cls.existing_path(value, name)
    if not path.is_dir():
      raise argparse.ArgumentTypeError(
          f"{name} is not a folder: '{repr(str(path))}'")
    return path

  @classmethod
  def non_empty_dir_path(cls,
                         value: pth.AnyPathLike,
                         name: str = "value") -> pth.LocalPath:
    dir_path = cls.dir_path(value, name)
    for _ in dir_path.iterdir():
      return dir_path
    raise argparse.ArgumentTypeError(
        f"{name} dir must be non empty: {repr(str(dir_path))}")

  @classmethod
  def existing_path(cls,
                    value: pth.AnyPathLike,
                    name: str = "value") -> pth.LocalPath:
    path = cls.path(value)
    if not path.exists():
      raise argparse.ArgumentTypeError(
          f"{name} path does not exist: {repr(str(path))}")
    return path

  @classmethod
  def not_existing_path(cls,
                        value: pth.AnyPathLike,
                        name: str = "value") -> pth.LocalPath:
    path = cls.path(value)
    if path.exists():
      raise argparse.ArgumentTypeError(
          f"{name} path already exists: {repr(str(path))}")
    return path

  @classmethod
  def binary_path(cls,
                  value: Optional[pth.AnyPathLike],
                  name: str = "binary",
                  platform: Optional[plt.Platform] = None) -> pth.AnyPath:
    platform = platform or plt.PLATFORM
    not_none: pth.AnyPathLike = ObjectParser.not_none(value,
                                                      name)  # type: ignore
    maybe_path: pth.AnyPath = platform.path(not_none)
    if platform.is_file(maybe_path):
      return maybe_path
    if maybe_bin := platform.search_binary(maybe_path):
      return maybe_bin
    raise argparse.ArgumentTypeError(f"Unknown binary: {value}")

  @classmethod
  def any_path(cls,
               value: Optional[pth.AnyPathLike],
               name: str = "value") -> pth.AnyPath:
    """Parse a path than can be on a local or remote file system."""
    if some_value := ObjectParser.not_none(value, name):
      return pth.AnyPath(some_value)  # type: ignore
    raise argparse.ArgumentTypeError(f"Expected non empty path {name}.")

  @classmethod
  def local_binary_path(cls,
                        value: Optional[pth.AnyPathLike],
                        name: str = "binary") -> pth.LocalPath:
    return cast(pth.LocalPath, cls.binary_path(value, name))

  @classmethod
  def json_file_path(cls, value: pth.AnyPathLike) -> pth.LocalPath:
    path = cls.file_path(value)
    with path.open(encoding="utf-8") as f:
      try:
        json.load(f)
      except ValueError as e:
        message = _extract_decoding_error(f"Invalid json file '{path}':", path,
                                          e)
        raise argparse.ArgumentTypeError(message) from e
    return path

  @classmethod
  def hjson_file_path(cls, value: pth.AnyPathLike) -> pth.LocalPath:
    path = cls.file_path(value)
    with path.open(encoding="utf-8") as f:
      try:
        hjson.load(f)
      except ValueError as e:
        message = _extract_decoding_error("Invalid hjson file '{path}':", path,
                                          e)
        raise argparse.ArgumentTypeError(message) from e
    return path


EnumT = TypeVar("EnumT", bound=enum.Enum)
NotNoneT = TypeVar("NotNoneT")
SequenceT = TypeVar("SequenceT", bound=Sequence)


class ObjectParser:

  @classmethod
  def enum(cls, label: str, enum_cls: Type[EnumT], data: Any,
           choices: Union[Type[EnumT], Iterable[EnumT]]) -> EnumT:
    try:
      # Try direct conversion, relying on the Enum._missing_ hook:
      enum_value = enum_cls(data)
      assert isinstance(enum_value, enum.Enum)
      assert isinstance(enum_value, enum_cls)
      return enum_value
    except Exception as e:  # pylint: disable=broad-except
      logging.debug("Could not auto-convert data '%s' to enum %s: %s", data,
                    enum_cls, e)

    for enum_instance in choices:
      if data in (enum_instance, enum_instance.value):
        return enum_instance
    choices_str: str = ", ".join(repr(item.value) for item in choices)  # pytype: disable=missing-parameter
    raise argparse.ArgumentTypeError(f"Unknown {label}: {repr(data)}.\n"
                                     f"Choices are {choices_str}.")

  @classmethod
  def inline_hjson(cls, value: Any) -> Any:
    value_str = cls.non_empty_str(value, "hjson")
    if value_str[0] != "{" or value_str[-1] != "}":
      raise argparse.ArgumentTypeError(
          "Invalid inline hjson, missing braces: '{value_str}'")
    try:
      return hjson.loads(value_str)
    except ValueError as e:
      message = _extract_decoding_error("Could not decode inline hjson",
                                        value_str, e)
      if "eof" in message:
        message += "\n   Likely missing quotes."
      raise argparse.ArgumentTypeError(message) from e

  @classmethod
  def json_file(cls, value: pth.AnyPathLike) -> Any:
    path = PathParser.file_path(value)
    with path.open(encoding="utf-8") as f:
      try:
        return json.load(f)
      except ValueError as e:
        message = _extract_decoding_error(f"Invalid json file '{path}':", path,
                                          e)
        raise argparse.ArgumentTypeError(message) from e

  @classmethod
  def hjson_file(cls, value: pth.AnyPathLike) -> Any:
    path = PathParser.file_path(value)
    with path.open(encoding="utf-8") as f:
      try:
        return hjson.load(f)
      except ValueError as e:
        message = _extract_decoding_error("Invalid hjson file '{path}':", path,
                                          e)
        raise argparse.ArgumentTypeError(message) from e

  @classmethod
  def non_empty_hjson_file(cls, value: pth.AnyPathLike) -> Any:
    data = cls.hjson_file(value)
    if not data:
      raise argparse.ArgumentTypeError(
          "Expected hjson file with non-empty data, "
          f"but got: {hjson.dumps(data)}")
    return data

  @classmethod
  def dict_hjson_file(cls, value: pth.AnyPathLike) -> Any:
    data = cls.non_empty_hjson_file(value)
    if not isinstance(data, dict):
      raise argparse.ArgumentTypeError(
          "Expected object in hjson config '{value}', "
          f"but got {type_str(data)}: {repr(data)}")
    return data

  @classmethod
  def dict(cls, value: Any, name: str = "value") -> Dict:
    if isinstance(value, dict):
      return value
    raise argparse.ArgumentTypeError(
        f"Expected dict, but {name} is {type_str(value)}: {repr(value)}")

  @classmethod
  def non_empty_dict(cls, value: Any, name: str = "value") -> Dict:
    dict_value = cls.dict(value)
    if not dict_value:
      raise argparse.ArgumentTypeError(
          f"Expected {name} to be a non-empty dict.")
    return dict_value

  @classmethod
  def sequence(cls, value: Any, name: str = "value") -> Sequence[Any]:
    if isinstance(value, (list, tuple)):
      return value
    raise argparse.ArgumentTypeError(
        f"Expected sequence, but {name} is {type_str(value)}: {repr(value)}")

  @classmethod
  def non_empty_sequence(cls, value: Any, name: str = "value") -> Sequence[Any]:
    sequence_value = cls.sequence(value, name)
    if not sequence_value:
      raise argparse.ArgumentTypeError(
          f"Expected {name} to be a non-empty sequence.")
    return sequence_value

  @classmethod
  def any_str(cls, value: Any, name: str = "value") -> str:
    value = cls.not_none(value, name)
    if isinstance(value, str):
      return value
    raise argparse.ArgumentTypeError(
        f"Expected str, but got {type_str(value)}: {value}")

  @classmethod
  def non_empty_str(cls, value: Any, name: str = "value") -> str:
    value = cls.any_str(value, name)
    if not isinstance(value, str):
      raise argparse.ArgumentTypeError(
          f"Expected non-empty string {name}, "
          f"but got {type_str(value)}: {repr(value)}")
    if not value:
      raise argparse.ArgumentTypeError(f"Non-empty string {name} expected.")
    return value

  @classmethod
  def url_str(cls,
              value: str,
              name: str = "url",
              schemes: Optional[Sequence[str]] = None) -> str:
    cls.url(value, name, schemes)
    return value

  @classmethod
  def httpx_url_str(cls, value: Any, name: str = "url") -> str:
    cls.url(value, name, schemes=("http", "https"))
    return value

  @classmethod
  def base_url(cls, value: str, name: str = "url") -> urlparse.ParseResult:
    url_str: str = cls.non_empty_str(value, name)
    try:
      return urlparse.urlparse(url_str)
    except ValueError as e:
      raise argparse.ArgumentTypeError(
          f"Invalid {name}: {repr(value)}, {e}") from e

  PORT_URL_PATH_RE = re.compile(r"^[0-9]+(?:/|$)")

  @classmethod
  def parse_fuzzy_url_str(cls,
                          value: str,
                          name: str = "url",
                          schemes: Sequence[str] = ("http", "https", "about",
                                                    "file", "data"),
                          default_scheme: str = "https") -> str:
    parsed = cls.parse_fuzzy_url(value, name, schemes, default_scheme)
    return urlparse.urlunparse(parsed)

  @classmethod
  def parse_fuzzy_url(cls,
                      value: str,
                      name: str = "url",
                      schemes: Sequence[str] = ("http", "https", "about",
                                                "file", "data"),
                      default_scheme: str = "https") -> urlparse.ParseResult:
    assert default_scheme, "missing default scheme value"
    value = cls.non_empty_str(value, name)
    if PathParser.PATH_PREFIX.match(value):
      value = f"file://{value}"
    else:
      parsed = cls.base_url(value)
      if not parsed.scheme:
        value = f"{default_scheme}://{value}"
      # Check if this was a url without a scheme but with ports, which gets
      # "wrongly" parsed and the host ends up in result.scheme and port and path
      # are merged into result.path.
      if parsed.scheme not in schemes and not parsed.netloc:
        if cls.PORT_URL_PATH_RE.match(parsed.path):
          # foo.com:8080/test => https://foo.com:8080/test
          value = f"{default_scheme}://{value}"
      schemes = tuple(schemes) + (default_scheme,)
    return cls.url(value, name, schemes)

  @classmethod
  def url(cls,
          value: str,
          name: str = "url",
          schemes: Optional[Sequence[str]] = None) -> urlparse.ParseResult:
    parsed = cls.base_url(value)
    try:
      scheme = parsed.scheme
      if schemes and scheme not in schemes:
        schemes_str = ",".join(map(repr, schemes))
        raise argparse.ArgumentTypeError(
            f"Invalid {name}: Expected scheme to be one of {schemes_str}, "
            f"but got {repr(parsed.scheme)} for url {repr(value)}")
      if port := parsed.port:
        _ = NumberParser.port_number(port, f"{name} port")
      if scheme in ("file", "about", "data"):
        return parsed
      hostname = parsed.hostname
      if not hostname:
        raise argparse.ArgumentTypeError(
            f"Missing hostname in {name}: {repr(value)}")
      if " " in hostname:
        raise argparse.ArgumentTypeError(
            f"Hostname in {name} contains invalid space: {repr(value)}")
    except ValueError as e:
      # Some ParseResult properties trigger errors, wrap all of them
      raise argparse.ArgumentTypeError(
          f"Invalid {name}: {repr(value)}, {e}") from e
    return parsed

  @classmethod
  def bool(cls, value: Any, name: str = "value") -> bool:
    if isinstance(value, bool):
      return value
    value = str(value).lower()
    if value == "true":
      return True
    if value == "false":
      return False
    raise argparse.ArgumentTypeError(
        f"Expected bool {name} but got {type_str(value)}: {repr(value)}")

  @classmethod
  def not_none(cls, value: Optional[NotNoneT], name: str = "value") -> NotNoneT:
    if value is None:
      raise argparse.ArgumentTypeError(f"Expected {name} to be not None.")
    return value

  @classmethod
  def sh_cmd(cls, value: Any) -> List[str]:
    value = cls.not_none(value, "shell cmd")
    if not value:
      raise argparse.ArgumentTypeError(
          f"Expected non-empty shell cmd, but got: {value}")
    if isinstance(value, (list, tuple)):
      for i, part in enumerate(value):
        cls.non_empty_str(part, f"cmd[{i}]")
      return list(value)
    if not isinstance(value, str):
      raise argparse.ArgumentTypeError(
          f"Expected string or list, but got {type_str(value)}: {value}")
    try:
      return shlex.split(value)
    except ValueError as e:
      raise argparse.ArgumentTypeError(f"Invalid shell cmd: {value} ") from e

  @classmethod
  def unique_sequence(
      cls,
      value: SequenceT,
      name: str = "sequence",
      error_cls: Type[Exception] = argparse.ArgumentTypeError) -> SequenceT:
    unique = set()
    duplicates = set()
    for item in value:
      if item in unique:
        duplicates.add(item)
      else:
        unique.add(item)
    if not duplicates:
      return value
    raise error_cls(f"Unexpected duplicates in {name}: {repr(duplicates)}")

  @classmethod
  def regexp(cls, value: Any, name: str = "regexp") -> re.Pattern:
    try:
      return re.compile(cls.any_str(value, name))
    except re.error as e:
      raise argparse.ArgumentTypeError(f"Invalid regexp {name}: {value}") from e

  @classmethod
  def safe_filename(cls, value: Any, name: str = "safe filename") -> str:
    return pth.safe_filename(cls.non_empty_str(value, name))


_MAX_LEN = 70


def _extract_decoding_error(message: str, value: pth.AnyPathLike,
                            e: ValueError) -> str:
  lineno = getattr(e, "lineno", -1) - 1
  colno = getattr(e, "colno", -1) - 1
  if lineno < 0 or colno < 0:
    if isinstance(value, pth.LocalPath):
      return f"{message}\n    {str(e)}"
    return f"{message}: {value}\n    {str(e)}"
  if isinstance(value, pth.AnyPath):
    with pth.LocalPath(value).open(encoding="utf-8") as f:
      line = f.readlines()[lineno]
  else:
    line = value.splitlines()[lineno]
  if len(line) > _MAX_LEN:
    # Only show line around error:
    start = colno - _MAX_LEN // 2
    end = colno + _MAX_LEN // 2
    prefix = "..."
    suffix = "..."
    if start < 0:
      start = 0
      end = _MAX_LEN
      prefix = ""
    elif end > len(line):
      end = len(line)
      start = len(line) - _MAX_LEN
      suffix = ""
    colno -= start
    line = prefix + line[start:end] + suffix
    marker_space = (" " * len(prefix)) + (" " * colno)
  else:
    marker_space = " " * colno
  marker = "_▲_"
  # Adjust line to be aligned with marker size
  line = (" " * (len(marker) // 2)) + line
  return f"{message}\n    {line}\n    {marker_space}{marker}\n({str(e)})"


class NumberParser:

  @classmethod
  def any_float(cls, value: Any, name: str = "float") -> float:
    try:
      return float(value)
    except ValueError as e:
      raise argparse.ArgumentTypeError(f"Invalid {name}: {repr(value)}") from e

  @classmethod
  def positive_zero_float(cls, value: Any, name: str = "float") -> float:
    value_f = cls.any_float(value, name)
    if not math.isfinite(value_f) or value_f < 0:
      raise argparse.ArgumentTypeError(
          f"Expected {name} >= 0, but got: {value_f}")
    return value_f

  @classmethod
  def any_int(cls,
              value: Any,
              name: str = "value",
              parse_str: bool = True) -> int:
    if (not parse_str and
        isinstance(value, str)) or (not isinstance(value, (int, float, str))):
      raise argparse.ArgumentTypeError(
          f"Expected integer {name}, but got {type_str(value)}: {repr(value)}")
    if isinstance(value, float) and not value.is_integer():
      raise argparse.ArgumentTypeError(f"Invalid integer {name}: {repr(value)}")
    try:
      return int(value)
    except ValueError as e:
      raise argparse.ArgumentTypeError(
          f"Invalid integer {name}: {repr(value)}") from e

  @classmethod
  def positive_zero_int(cls,
                        value: Any,
                        name: str = "value",
                        parse_str: bool = True) -> int:
    value_i = cls.any_int(value, name, parse_str)
    if value_i < 0:
      raise argparse.ArgumentTypeError(
          f"Expected integer {name} >= 0, but got: {value_i}")
    return value_i

  @classmethod
  def positive_int(cls,
                   value: Any,
                   name: str = "value",
                   parse_str: bool = True) -> int:
    value_i = cls.any_int(value, name, parse_str)
    if not math.isfinite(value_i) or value_i <= 0:
      raise argparse.ArgumentTypeError(
          f"Expected integer {name} > 0, but got: {value_i}")
    return value_i

  @classmethod
  def port_number(cls,
                  value: Any,
                  name: str = "port",
                  parse_str: bool = True) -> int:
    port = cls.any_int(value, name, parse_str)
    if 1 <= port <= 65535:
      return port
    raise argparse.ArgumentTypeError(
        f"Invalid Port: expected 1 <= {name} <= 65535, but got: {repr(port)}")


class LateArgumentError(argparse.ArgumentTypeError):
  """Signals argument parse errors after parser.parse_args().
  This is used to map errors back to the original argument, much like
  argparse.ArgumentError does internally. However, since this happens after
  the internal argument parsing we need this custom implementation to print
  more descriptive error messages.
  """

  def __init__(self, flag: str, message: str) -> None:
    super().__init__(message)
    self.flag = flag
    self.message = message


class DurationParseError(argparse.ArgumentTypeError):
  pass


class DurationParser:

  @classmethod
  def help(cls) -> str:
    return "'12.5' == '12.5s',  units=['ms', 's', 'm', 'h']"

  _DURATION_RE: Final[re.Pattern] = re.compile(
      r"(?P<value>(-?\d+(\.\d+)?)) ?(?P<unit>[a-z]+)?")

  @classmethod
  def _to_timedelta(cls, value: float, suffix: str) -> dt.timedelta:
    if suffix in {"ms", "millis", "milliseconds"}:
      return dt.timedelta(milliseconds=value)
    if suffix in {"s", "sec", "secs", "second", "seconds"}:
      return dt.timedelta(seconds=value)
    if suffix in {"m", "min", "mins", "minute", "minutes"}:
      return dt.timedelta(minutes=value)
    if suffix in {"h", "hrs", "hour", "hours"}:
      return dt.timedelta(hours=value)
    raise DurationParseError(f"Error: {suffix} is not supported for duration. "
                             "Make sure to use a supported time unit/suffix")

  @classmethod
  def positive_duration(cls,
                        time_value: Any,
                        name: str = "duration") -> dt.timedelta:
    duration: dt.timedelta = cls.any_duration(time_value)
    if duration.total_seconds() <= 0:
      raise DurationParseError(f"Expected non-zero {name}, but got {duration}")
    return duration

  @classmethod
  def positive_or_zero_duration(cls,
                                time_value: Any,
                                name: str = "duration") -> dt.timedelta:
    duration: dt.timedelta = cls.any_duration(time_value, name)
    if duration.total_seconds() < 0:
      raise DurationParseError(f"Expected positive {name}, but got {duration}")
    return duration

  @classmethod
  def any_duration(cls,
                   time_value: Any,
                   name: str = "duration") -> dt.timedelta:
    """
    This function will parse the measurement and the value from string value.

    For example:
    5s => dt.timedelta(seconds=5)
    5m => 5*60 = dt.timedelta(minutes=5)

    """
    if isinstance(time_value, dt.timedelta):
      return time_value
    if isinstance(time_value, (int, float)):
      return dt.timedelta(seconds=time_value)
    if not time_value:
      raise DurationParseError(f"Expected non-empty {name} value.")
    if not isinstance(time_value, str):
      raise DurationParseError(
          f"Unexpected {type_str(time_value)} for {name}: {time_value}")

    match = cls._DURATION_RE.fullmatch(time_value)
    if match is None:
      raise DurationParseError(f"Unknown {name} format: '{time_value}'")

    value = match.group("value")
    if not value:
      raise DurationParseError(
          f"Error: {name} value not found."
          f"Make sure to include a valid {name} value: '{time_value}'")
    time_unit = match.group("unit")
    try:
      time_value = float(value)
    except ValueError as e:
      raise DurationParseError(f"{name} must be a valid number, {e}") from e
    if not math.isfinite(time_value):
      raise DurationParseError(f"{name} must be finite, but got: {time_value}")

    if not time_unit:
      # If no time unit provided we assume it is in seconds.
      return dt.timedelta(seconds=time_value)
    return cls._to_timedelta(time_value, time_unit)
