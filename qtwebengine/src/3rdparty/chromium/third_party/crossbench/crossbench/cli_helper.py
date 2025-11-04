# Copyright 2023 The Chromium Authors
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

from __future__ import annotations

import argparse
import contextlib
import datetime as dt
import enum
import json
import logging
import math
import re
import shlex
import sys
from typing import (Any, Dict, Final, Iterable, Iterator, List, Optional,
                    Sequence, Type, TypeVar, Union, cast)
from urllib import parse as urlparse

import colorama
import hjson

from crossbench import helper
from crossbench import path as pth
from crossbench import plt


def type_str(value: Any) -> str:
  return type(value).__name__


def parse_path(value: pth.RemotePathLike, name: str = "value") -> pth.LocalPath:
  value = parse_not_none(value, "path")
  if not value:
    raise argparse.ArgumentTypeError("Invalid empty path.")
  try:
    path = pth.LocalPath(value).expanduser()
  except RuntimeError as e:
    raise argparse.ArgumentTypeError(
        f"Invalid Path {name} {repr(value)}': {e}") from e
  return path


def parse_existing_file_path(value: pth.RemotePathLike,
                             name: str = "value") -> pth.LocalPath:
  path = parse_existing_path(value, name)
  if not path.is_file():
    raise argparse.ArgumentTypeError(f"{name} is not a file: {repr(str(path))}")
  return path


def parse_non_empty_file_path(value: pth.RemotePathLike,
                              name: str = "value") -> pth.LocalPath:
  path: pth.LocalPath = parse_existing_file_path(value, name)
  if path.stat().st_size == 0:
    raise argparse.ArgumentTypeError(
        f"{name} is an empty file: {repr(str(path))}")
  return path


def parse_file_path(value: pth.RemotePathLike,
                    name: str = "value") -> pth.LocalPath:
  return parse_non_empty_file_path(value, name)


def parse_dir_path(value: pth.RemotePathLike,
                   name: str = "value") -> pth.LocalPath:
  path = parse_existing_path(value, name)
  if not path.is_dir():
    raise argparse.ArgumentTypeError(
        f"{name} is not a folder: '{repr(str(path))}'")
  return path


def parse_non_empty_dir_path(value: pth.RemotePathLike,
                             name: str = "value") -> pth.LocalPath:
  dir_path = parse_dir_path(value, name)
  for _ in dir_path.iterdir():
    return dir_path
  raise argparse.ArgumentTypeError(
      f"{name} dir must be non empty: {repr(str(dir_path))}")


def parse_existing_path(value: pth.RemotePathLike,
                        name: str = "value") -> pth.LocalPath:
  path = parse_path(value)
  if not path.exists():
    raise argparse.ArgumentTypeError(
        f"{name} path does not exist: {repr(str(path))}")
  return path


def parse_not_existing_path(value: pth.RemotePathLike,
                            name: str = "value") -> pth.LocalPath:
  path = parse_path(value)
  if path.exists():
    raise argparse.ArgumentTypeError(
        f"{name} path already exists: {repr(str(path))}")
  return path


def parse_binary_path(
    value: Optional[pth.RemotePathLike],
    name: str = "binary",
    platform: Optional[plt.Platform] = None) -> pth.RemotePath:
  platform = platform or plt.PLATFORM
  maybe_path = platform.path(parse_not_none(value, name))
  if platform.is_file(maybe_path):
    return maybe_path
  maybe_bin = platform.search_binary(maybe_path)
  if not maybe_bin:
    raise argparse.ArgumentTypeError(f"Unknown binary: {value}")
  return maybe_bin


def parse_remote_path(value: Optional[pth.RemotePathLike],
                      name: str = "value") -> pth.RemotePath:
  some_value: pth.RemotePathLike = parse_not_none(value, name)
  if not some_value:
    raise argparse.ArgumentTypeError(f"Expected non empty path {name}.")
  return pth.RemotePath(some_value)


def parse_local_binary_path(
    value: Optional[pth.RemotePathLike],
    name: str = "binary") -> pth.LocalPath:
  return cast(pth.LocalPath, parse_binary_path(value, name))


EnumT = TypeVar("EnumT", bound=enum.Enum)


def parse_enum(label: str, enum_cls: Type[EnumT], data: Any,
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


def parse_inline_hjson(value: Any) -> Any:
  value_str = parse_non_empty_str(value, hjson.__name__)
  if value_str[0] != "{" or value_str[-1] != "}":
    raise argparse.ArgumentTypeError(
        f"Invalid inline {hjson.__name__}, missing braces: '{value_str}'")
  try:
    return hjson.loads(value_str)
  except ValueError as e:
    message = _extract_decoding_error(
        f"Could not decode inline {hjson.__name__}", value_str, e)
    if "eof" in message:
      message += "\n   Likely missing quotes."
    raise argparse.ArgumentTypeError(message) from e


_MAX_LEN = 70


def _extract_decoding_error(message: str, value: pth.RemotePathLike,
                            e: ValueError) -> str:
  lineno = getattr(e, "lineno", -1) - 1
  colno = getattr(e, "colno", -1) - 1
  if lineno < 0 or colno < 0:
    if isinstance(value, pth.LocalPath):
      return f"{message}\n    {str(e)}"
    return f"{message}: {value}\n    {str(e)}"
  if isinstance(value, pth.RemotePath):
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


def parse_json_file_path(value: pth.RemotePathLike) -> pth.LocalPath:
  path = parse_file_path(value)
  with path.open(encoding="utf-8") as f:
    try:
      json.load(f)
    except ValueError as e:
      message = _extract_decoding_error(f"Invalid json file '{path}':", path, e)
      raise argparse.ArgumentTypeError(message) from e
  return path


def parse_hjson_file_path(value: pth.RemotePathLike) -> pth.LocalPath:
  path = parse_file_path(value)
  with path.open(encoding="utf-8") as f:
    try:
      hjson.load(f)
    except ValueError as e:
      message = _extract_decoding_error(
          f"Invalid {hjson.__name__} file '{path}':", path, e)
      raise argparse.ArgumentTypeError(message) from e
  return path


def parse_json_file(value: pth.RemotePathLike) -> Any:
  path = parse_file_path(value)
  with path.open(encoding="utf-8") as f:
    try:
      return json.load(f)
    except ValueError as e:
      message = _extract_decoding_error(f"Invalid json file '{path}':", path, e)
      raise argparse.ArgumentTypeError(message) from e


def parse_hjson_file(value: pth.RemotePathLike) -> Any:
  path = parse_file_path(value)
  with path.open(encoding="utf-8") as f:
    try:
      return hjson.load(f)
    except ValueError as e:
      message = _extract_decoding_error(
          f"Invalid {hjson.__name__} file '{path}':", path, e)
      raise argparse.ArgumentTypeError(message) from e


def parse_non_empty_hjson_file(value: pth.RemotePathLike) -> Any:
  data = parse_hjson_file(value)
  if not data:
    raise argparse.ArgumentTypeError(
        f"Expected {hjson.__name__} file with non-empty data, "
        f"but got: {hjson.dumps(data)}")
  return data


def parse_dict_hjson_file(value: pth.RemotePathLike) -> Any:
  data = parse_non_empty_hjson_file(value)
  if not isinstance(data, dict):
    raise argparse.ArgumentTypeError(
        f"Expected object in {hjson.__name__} config '{value}', "
        f"but got {type_str(data)}: {repr(data)}")
  return data


def parse_dict(value: Any, name: str = "value") -> Dict:
  if isinstance(value, dict):
    return value
  raise argparse.ArgumentTypeError(
      f"Expected dict, but {name} is {type_str(value)}: {repr(value)}")


def parse_non_empty_dict(value: Any, name: str = "value") -> Dict:
  dict_value = parse_dict(value)
  if not dict_value:
    raise argparse.ArgumentTypeError(f"Expected {name} to be a non-empty dict.")
  return dict_value


def parse_sequence(value: Any, name: str = "value") -> Sequence[Any]:
  if isinstance(value, (list, tuple)):
    return value
  raise argparse.ArgumentTypeError(
      f"Expected sequence, but {name} is {type_str(value)}: {repr(value)}")


def parse_non_empty_sequence(value: Any, name: str = "value") -> Sequence[Any]:
  sequence_value = parse_sequence(value)
  if not sequence_value:
    raise argparse.ArgumentTypeError(
        f"Expected {name} to be a non-empty sequence.")
  return sequence_value


def try_resolve_existing_path(value: str) -> Optional[pth.LocalPath]:
  if not value:
    return None
  maybe_path = pth.LocalPath(value)
  if maybe_path.exists():
    return maybe_path
  maybe_path = maybe_path.expanduser()
  if maybe_path.exists():
    return maybe_path
  return None


def parse_float(value: Any, name: str = "float") -> float:
  try:
    return float(value)
  except ValueError as e:
    raise argparse.ArgumentTypeError(f"Invalid {name}: {repr(value)}") from e


def parse_positive_zero_float(value: Any, name: str = "float") -> float:
  value_f = parse_float(value, name)
  if not math.isfinite(value_f) or value_f < 0:
    raise argparse.ArgumentTypeError(
        f"Expected {name} >= 0, but got: {value_f}")
  return value_f


def parse_int(value: Any, name: str = "value") -> int:
  try:
    return int(value)
  except ValueError as e:
    raise argparse.ArgumentTypeError(
        f"Invalid integer {name}: {repr(value)}") from e


def parse_positive_zero_int(value: Any, name: str = "value") -> int:
  value_i = parse_int(value, name)
  if value_i < 0:
    raise argparse.ArgumentTypeError(
        f"Expected integer {name} >= 0, but got: {value_i}")
  return value_i


def parse_positive_int(value: Any, name: str = "value") -> int:
  value_i = parse_int(value, name)
  if not math.isfinite(value_i) or value_i <= 0:
    raise argparse.ArgumentTypeError(
        f"Expected integer {name} > 0, but got: {value_i}")
  return value_i


def parse_port(value: Any, name: str = "port") -> int:
  port = parse_int(value, name)
  if 1 <= port <= 65535:
    return port
  raise argparse.ArgumentTypeError(
      f"Invalid Port: expected 1 <= {name} <= 65535, but got: {repr(port)}")


def parse_str(value: Any, name: str = "value") -> str:
  value = parse_not_none(value, name)
  if isinstance(value, str):
    return value
  raise argparse.ArgumentTypeError(
      f"Expected str, but got {type_str(value)}: {value}")


def parse_non_empty_str(value: Any, name: str = "value") -> str:
  value = parse_str(value, name)
  if not isinstance(value, str):
    raise argparse.ArgumentTypeError(
        f"Expected non-empty string {name}, "
        f"but got {type_str(value)}: {repr(value)}")
  if not value:
    raise argparse.ArgumentTypeError(f"Non-empty string {name} expected.")
  return value


def parse_url_str(value: str,
                  name: str = "url",
                  schemes: Optional[Sequence[str]] = None) -> str:
  parse_url(value, name, schemes)
  return value


def parse_httpx_url_str(value: Any, name: str = "url") -> str:
  parse_url(value, name, schemes=("http", "https"))
  return value


def parse_base_url(value: str, name: str = "url") -> urlparse.ParseResult:
  url_str: str = parse_non_empty_str(value, name)
  try:
    return urlparse.urlparse(url_str)
  except ValueError as e:
    raise argparse.ArgumentTypeError(
        f"Invalid {name}: {repr(value)}, {e}") from e


PATH_PREFIX = re.compile(r"^(?:"
                         r"(?:\.\.?|~)?|"
                         r"[a-zA-Z]:"
                         r")(\\|/)[^\\/]")
PORT_URL_PATH_RE = re.compile(r"^[0-9]+(?:/|$)")


def parse_fuzzy_url_str(value: str,
                        name: str = "url",
                        schemes: Sequence[str] = ("http", "https", "about",
                                                  "file"),
                        default_scheme: str = "https") -> str:
  parsed = parse_fuzzy_url(value, name, schemes, default_scheme)
  return urlparse.urlunparse(parsed)


def parse_fuzzy_url(value: str,
                    name: str = "url",
                    schemes: Sequence[str] = ("http", "https", "about", "file"),
                    default_scheme: str = "https") -> urlparse.ParseResult:
  assert default_scheme, "missing default scheme value"
  value = parse_non_empty_str(value, name)
  if PATH_PREFIX.match(value):
    value = f"file://{value}"
  else:
    parsed = parse_base_url(value)
    if not parsed.scheme:
      value = f"{default_scheme}://{value}"
    # Check if this was a url without a scheme but with ports, which gets
    # "wrongly" parsed and the host ends up in result.scheme and port and path
    # are merged into result.path.
    if parsed.scheme not in schemes and not parsed.netloc:
      if PORT_URL_PATH_RE.match(parsed.path):
        # foo.com:8080/test => https://foo.com:8080/test
        value = f"{default_scheme}://{value}"
    schemes = tuple(schemes) + (default_scheme,)
  return parse_url(value, name, schemes)


def parse_url(value: str,
              name: str = "url",
              schemes: Optional[Sequence[str]] = None) -> urlparse.ParseResult:
  parsed = parse_base_url(value)
  try:
    scheme = parsed.scheme
    if schemes and scheme not in schemes:
      schemes_str = ",".join(map(repr, schemes))
      raise argparse.ArgumentTypeError(
          f"Invalid {name}: Expected scheme to be one of {schemes_str}, "
          f"but got {repr(parsed.scheme)} for url {repr(value)}")
    if port := parsed.port:
      _ = parse_port(port, f"{name} port")
    if scheme in ("file", "about"):
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


def parse_bool(value: Any, name: str = "value") -> bool:
  if isinstance(value, bool):
    return value
  value = str(value).lower()
  if value == "true":
    return True
  if value == "false":
    return False
  raise argparse.ArgumentTypeError(
      f"Expected bool {name} but got {type_str(value)}: {repr(value)}")


NotNoneT = TypeVar("NotNoneT")


def parse_not_none(value: Optional[NotNoneT], name: str = "value") -> NotNoneT:
  if value is None:
    raise argparse.ArgumentTypeError(f"Expected {name} to be not None.")
  return value


def parse_sh_cmd(value: Any) -> List[str]:
  value = parse_not_none(value, "shell cmd")
  if not value:
    raise argparse.ArgumentTypeError(
        f"Expected non-empty shell cmd, but got: {value}")
  if isinstance(value, (list, tuple)):
    for i, part in enumerate(value):
      parse_non_empty_str(part, f"cmd[{i}]")
    return list(value)
  if not isinstance(value, str):
    raise argparse.ArgumentTypeError(
        f"Expected string or list, but got {type_str(value)}: {value}")
  try:
    return shlex.split(value)
  except ValueError as e:
    raise argparse.ArgumentTypeError(f"Invalid shell cmd: {value} ") from e


SequenceT = TypeVar("SequenceT", bound=Sequence)


def parse_unique_sequence(
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


@contextlib.contextmanager
def late_argument_type_error_wrapper(flag: str) -> Iterator[None]:
  """Converts raised ValueError and ArgumentTypeError to LateArgumentError
  that are associated with the given flag.
  """
  try:
    yield
  except Exception as e:
    raise LateArgumentError(flag, str(e)) from e


class DurationParseError(argparse.ArgumentTypeError):
  pass


class Duration:

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
  def parse(cls, time_value: Any, name: str = "duration") -> dt.timedelta:
    return cls.parse_non_zero(time_value, name)

  @classmethod
  def parse_non_zero(cls,
                     time_value: Any,
                     name: str = "duration") -> dt.timedelta:
    duration: dt.timedelta = cls.parse_any(time_value)
    if duration.total_seconds() <= 0:
      raise DurationParseError(f"Expected non-zero {name}, but got {duration}")
    return duration

  @classmethod
  def parse_zero(cls, time_value: Any, name: str = "duration") -> dt.timedelta:
    duration: dt.timedelta = cls.parse_any(time_value, name)
    if duration.total_seconds() < 0:
      raise DurationParseError(f"Expected positive {name}, but got {duration}")
    return duration

  @classmethod
  def parse_any(cls, time_value: Any, name: str = "duration") -> dt.timedelta:
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


@contextlib.contextmanager
def timer(msg: str = "Elapsed Time"):
  start_time = dt.datetime.now()

  def print_timer():
    delta = dt.datetime.now() - start_time
    indent = colorama.Cursor.FORWARD() * 3
    sys.stdout.write(f"{indent}{msg}: {delta}\r")

  with helper.RepeatTimer(interval=0.25, function=print_timer):
    yield
