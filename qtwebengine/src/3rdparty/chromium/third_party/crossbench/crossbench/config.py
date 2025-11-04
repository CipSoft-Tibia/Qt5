# Copyright 2022 The Chromium Authors
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

from __future__ import annotations

import abc
import argparse
import collections
import collections.abc
import enum
import inspect
import logging
import re
import textwrap
from typing import (TYPE_CHECKING, Any, Callable, Dict, Generic, Iterable,
                    List, Optional, Set, Tuple, Type, TypeVar, Union, cast)
from urllib.parse import urlparse

import tabulate

# Use indirection to support pyfakefs
from crossbench import cli_helper, compat, exception, helper
from crossbench import path as pth
from crossbench.helper import ChangeCWD

if TYPE_CHECKING:
  ArgParserType = Union[Callable[..., Any], Type]


class ConfigError(argparse.ArgumentTypeError):
  pass

class _ConfigArgParser:
  """
  Parser for a single config arg.
  """

  def __init__(  # pylint: disable=redefined-builtin
      self,
      parser: ConfigParser,
      name: str,
      type: Optional[ArgParserType],
      default: Any = None,
      choices: Optional[frozenset[Any]] = None,
      aliases: Iterable[str] = tuple(),
      help: Optional[str] = None,
      is_list: bool = False,
      required: bool = False,
      depends_on: Optional[Iterable[str]] = None):
    self.parser: ConfigParser = parser
    self.name: str = name
    self.aliases = tuple(aliases)
    self._validate_aliases()
    self.type: Optional[ArgParserType] = type
    self.default = default
    self.help: Optional[str] = help
    self.is_list: bool = is_list
    self.required: bool = required
    type_is_class = inspect.isclass(type)
    self.type_is_class: bool = type_is_class
    self.is_enum: bool = type_is_class and issubclass(type, enum.Enum)
    self.config_object_type: Optional[Type[ConfigObject]] = None
    if type_is_class and issubclass(type, ConfigObject):
      self.config_object_type = type
    self.depends_on = frozenset(depends_on) if depends_on else frozenset()
    self.choices: Optional[frozenset] = self._validate_choices(choices)
    if self.type:
      self._validate_callable()
    if self.default is not None:
      self._validate_default()
    self._validate_depends_on(depends_on)

  def _validate_callable(self) -> None:
    assert self.type, "Expected not-None type"
    if not callable(self.type):
      raise TypeError(
          f"Expected type to be a class or a callable, but got: {self.type}")
    if self.config_object_type:
      # Config objects and depends_on are handled specially.
      return

    signature = None
    if getattr(self.type, "__module__") != "builtins":
      try:
        signature = inspect.signature(self.type)
      except ValueError as e:
        logging.debug("Could not get signature for %s: %s", self.type, e)

    if not signature:
      if not self.depends_on:
        return
      raise TypeError(
          f"Type for config '{self.name}' should take at least 2 arguments "
          f"to support depends_on, but got builtin: {self.type}")

    if len(signature.parameters) == 0:
      raise TypeError(
          f"Type for config '{self.name}' should take at least 1 argument, "
          f"but got: {self.type}")
    if self.depends_on and len(signature.parameters) <= 1:
      raise TypeError(
          f"Type for config '{self.name}' should take at least 2 arguments "
          f"to support depends_on, but got: {self.type}")

  def _validate_aliases(self) -> None:
    unique = set(self.aliases)
    if self.name in unique:
      raise ValueError(f"Config name '{self.name}' cannot be part "
                       f"of the aliases='{self.aliases}'")
    cli_helper.parse_unique_sequence(self.aliases, "aliases", ValueError)

  def _validate_choices(
      self, choices: Optional[frozenset[Any]]) -> Optional[frozenset]:
    if self.is_enum:
      return self._validate_enum_choices(choices)
    if choices is None:
      return None
    choices_list = list(choices)
    assert choices_list, f"Got empty choices: {choices}"
    frozen_choices = frozenset(choices_list)
    if len(frozen_choices) != len(choices_list):
      raise ValueError("Choices must be unique, but got: {choices}")
    return frozen_choices

  def _validate_enum_choices(
      self, choices: Optional[frozenset[Any]]) -> Optional[frozenset]:
    assert self.is_enum
    assert self.type
    enum_type: Type[enum.Enum] = cast(Type[enum.Enum], self.type)
    if choices is None:
      return frozenset(enum for enum in enum_type)
    for choice in choices:
      assert isinstance(
          choice,
          enum_type), (f"Enum choices must be {enum_type}, but got: {choice}")
    return frozenset(choices)

  def _validate_default(self) -> None:
    if self.is_enum:
      self._validate_enum_default()
      return
    # TODO: Remove once pytype can handle self.type
    maybe_class: Optional[ArgParserType] = self.type
    if self.is_list:
      assert isinstance(self.default, collections.abc.Sequence), (
          f"List default must be a sequence, but got: {self.default}")
      assert not isinstance(self.default, str), (
          f"List default should not be a string, but got: {repr(self.default)}")
      if inspect.isclass(maybe_class):
        for default_item in self.default:
          if not isinstance(default_item, maybe_class):
            raise ValueError(
                f"Expected default list item of type={self.type}, "
                f"but got type={type(default_item)}: {default_item}")
    elif maybe_class and inspect.isclass(maybe_class):
      if not isinstance(self.default, maybe_class):
        raise ValueError(f"Expected default value of type={self.type}, "
                         f"but got type={type(self.default)}: {self.default}")

  def _validate_enum_default(self) -> None:
    enum_type: Type[enum.Enum] = cast(Type[enum.Enum], self.type)
    if self.is_list:
      default_list = self.default
    else:
      default_list = [self.default]
    for default in default_list:
      assert isinstance(default, enum_type), (
          f"Default must be a {enum_type} enum, but got: {self.default}")

  def _validate_depends_on(self, depends_on: Optional[Iterable[str]]) -> None:
    if not depends_on:
      return
    if not self._is_iterable_non_str(depends_on):
      raise TypeError(f"Expected depends_on to be a collection of str, "
                      f"but got {type(depends_on).__name__}: "
                      f"{repr(depends_on)}")
    for i, value in enumerate(depends_on):
      if not isinstance(value, str):
        raise TypeError(f"Expected depends_on[{i}] to be a str, but got "
                        f"{type(value).__name__}: {repr(value)}")
    if not self.type:
      raise ValueError(f"Argument '{self.name}' without a type "
                       "cannot have argument dependencies.")
    if self.is_enum:
      raise ValueError(f"Enum '{self.name}' cannot have argument dependencies")

  def _is_iterable_non_str(self, value: Any) -> bool:
    if isinstance(value, str):
      return False
    return isinstance(value, collections.abc.Iterable)

  @property
  def cls(self) -> Type:
    return self.parser.cls

  @property
  def cls_name(self) -> str:
    return self.cls.__name__

  @property
  def help_text(self) -> str:
    items: List[Tuple[str, str]] = []
    if self.type is None:
      if self.is_list:
        items.append(("type", "list"))
    else:
      if self.is_list:
        items.append(("type", f"List[{self.type.__qualname__}]"))
      else:
        items.append(("type", str(self.type.__qualname__)))

    if self.required:
      items.append(("required", ""))
    elif self.default is None:
      items.append(("default", "not set"))
    else:
      if self.is_list:
        if not self.default:
          items.append(("default", "[]"))
        else:
          items.append(("default", ",".join(map(str, self.default))))
      else:
        items.append(("default", str(self.default)))
    if self.is_enum:
      items.extend(self._enum_help_text())
    elif self.choices:
      items.append(self._choices_help_text(self.choices))

    text = tabulate.tabulate(items, tablefmt="presto")
    if self.help:
      return f"{self.help}\n{text}"
    return text

  def _choices_help_text(self, choices: Iterable) -> Tuple[str, str]:
    return ("choices", ", ".join(map(str, choices)))

  def _enum_help_text(self) -> List[Tuple[str, str]]:
    if self.type and hasattr(self.type, "help_text_items"):
      # See compat.StrEnumWithHelp
      return [("choices", ""), *self.type.help_text_items()]
    assert self.choices
    return [self._choices_help_text(choice.value for choice in self.choices)]

  def parse(self, config_data: Dict[str, Any],
            depending_kwargs: Dict[str, Any]) -> Any:
    data = None
    if self.name in config_data:
      data = config_data.pop(self.name)
    elif self.aliases:
      data = self._pop_alias(config_data)

    if data is None:
      if self.required and self.default is None:
        raise ValueError(
            f"{self.cls_name}: "
            f"No value provided for required config option '{self.name}'")
      data = self.default
      if depending_kwargs:
        self._validate_depending_kwargs(depending_kwargs)
    else:
      self._validate_depending_kwargs(depending_kwargs)
      self._validate_no_aliases(config_data)
    if data is None and not depending_kwargs:
      return None
    if self.is_list:
      return self.parse_list_data(data, depending_kwargs)
    return self.parse_data(data, depending_kwargs)

  def _pop_alias(self, config_data) -> Optional[Any]:
    value: Optional[Any] = None
    found: bool = False
    for alias in self.aliases:
      if alias not in config_data:
        continue
      if found:
        raise ValueError(f"Ambiguous arguments, got alias for {self.name} "
                         "specified more than once.")
      value = config_data.pop(alias, None)
      found = True
    return value

  def _validate_depending_kwargs(self, depending_kwargs: Dict[str,
                                                              Any]) -> None:
    if not self.depends_on and depending_kwargs:
      raise ValueError(f"{self.name} has no depending arguments, "
                       f"but got: {depending_kwargs}")
    for arg_name in self.depends_on:
      if arg_name not in depending_kwargs:
        raise ValueError(
            f"{arg_name}.depends_on['{arg_name}'] was not provided.")

  def _validate_no_aliases(self, config_data) -> None:
    for alias in self.aliases:
      if alias in config_data:
        raise ValueError(
            f"{self.cls_name}: ",
            f"Got conflicting argument, '{self.name}' and '{alias}' "
            "cannot be specified together.")

  def _validate_type_without_depending_kwargs(
      self, depending_kwargs: Dict[str, Any]) -> None:
    if depending_kwargs:
      raise ValueError(
          f"{str(self.type)} does not accept "
          f"additional depending arguments, but got: {depending_kwargs}")

  def parse_list_data(self, data: Any,
                      depending_kwargs: Dict[str, Any]) -> List[Any]:
    if isinstance(data, str):
      data = data.split(",")
    if not isinstance(data, (list, tuple)):
      raise ValueError(f"{self.cls_name}.{self.name}: "
                       f"Expected sequence got {type(data)}")
    return [self.parse_data(value, depending_kwargs) for value in data]

  def parse_data(self, data: Any, depending_kwargs: Dict[str, Any]) -> Any:
    if self.is_enum:
      self._validate_type_without_depending_kwargs(depending_kwargs)
      return self.parse_enum_data(data)
    if self.choices and data not in self.choices:
      raise ValueError(f"{self.cls_name}.{self.name}: "
                       f"Invalid choice '{data}', choices are {self.choices}")
    if self.type is None:
      self._validate_type_without_depending_kwargs(depending_kwargs)
      return data
    if self.type is bool:
      self._validate_type_without_depending_kwargs(depending_kwargs)
      if not isinstance(data, bool):
        raise ValueError(
            f"{self.cls_name}.{self.name}: Expected bool, but got {data}")
    elif self.type in (float, int):
      self._validate_type_without_depending_kwargs(depending_kwargs)
      if not isinstance(data, (float, int)):
        raise ValueError(
            f"{self.cls_name}.{self.name}: Expected number, got {data}")
    if self.config_object_type:
      # TODO: support custom depending kwargs with ConfigObject
      self._validate_type_without_depending_kwargs(depending_kwargs)
      return self.parse_config_object(data)
    return self.type(data, **depending_kwargs)

  def parse_config_object(self, data) -> Any:
    config_object: ConfigObject = self.config_object_type.parse(data)
    return config_object.to_argument_value()

  def parse_enum_data(self, data: Any) -> enum.Enum:
    assert self.is_enum
    assert self.choices
    assert self.type
    assert isinstance(self.type, type), "type for enum has to be a Class."
    if issubclass(self.type, ConfigEnum):
      return self.type.parse(data)
    assert issubclass(self.type, enum.Enum)
    return cli_helper.parse_enum(self.name, self.type, data, self.choices)




ConfigEnumT = TypeVar("ConfigEnumT", bound="ConfigEnum")


class ConfigEnum(compat.StrEnumWithHelp):

  @classmethod
  def parse(cls: Type[ConfigEnumT], value: Any) -> ConfigEnumT:
    return cli_helper.parse_enum(cls.__name__, cls, value, cls)


ConfigObjectT = TypeVar("ConfigObjectT", bound="ConfigObject")

class ConfigObject(abc.ABC):
  """A ConfigObject is a placeholder object with parsed values from
  a ConfigParser.
  - It is used to do complex input validation when the final instantiated
    objects contain other nested config-parsed objects,
  - It is then used to create a real instance of an object.
  """
  VALID_EXTENSIONS: Tuple[str, ...] = (".hjson", ".json")

  @classmethod
  def value_has_path_prefix(cls, value: str) -> bool:
    return cli_helper.PATH_PREFIX.match(value) is not None

  def __post_init__(self) -> None:
    self.validate()

  def validate(self) -> None:
    """Override to perform validation of config properties that cannot be
    checked individually (aka depend on each other).
    """

  def to_argument_value(self) -> Any:
    """ Called to convert a ConfigObject to the value stored in ConfigParser
     result. """
    return self

  @classmethod
  def parse(cls: Type[ConfigObjectT], value: Any, **kwargs) -> ConfigObjectT:
    # Quick return for default values used by parsers.
    if isinstance(value, cls):
      return value
    # Make sure we wrap any exception in a argparse.ArgumentTypeError)
    with exception.annotate_argparsing(f"Parsing {cls.__name__}"):
      return cls._parse(value, **kwargs)

  @classmethod
  def _parse(cls: Type[ConfigObjectT], value: Any, **kwargs) -> ConfigObjectT:
    if isinstance(value, dict):
      return cls.parse_dict(value, **kwargs)
    if not value:
      raise ConfigError(f"{cls.__name__}: Empty config value")
    if isinstance(value, pth.LocalPath):
      return cls.parse_path(value, **kwargs)
    if isinstance(value, str):
      if urlparse(value).scheme:
        # TODO(346197734): use parse_url here
        return cls.parse_str(value, **kwargs)
      try:
        maybe_path = pth.LocalPath(value).expanduser()
        if cls.is_valid_path(maybe_path):
          return cls.parse_path(maybe_path, **kwargs)
        if cls.value_has_path_prefix(value):
          return cls.parse_unknown_path(maybe_path, **kwargs)
      except OSError:
        pass
      return cls.parse_str(value, **kwargs)
    return cls.parse_other(value, **kwargs)

  @classmethod
  def parse_other(cls: Type[ConfigObjectT], value: Any) -> ConfigObjectT:
    raise ConfigError(f"Invalid config input type {type(value)}: {value}")

  @classmethod
  @abc.abstractmethod
  def parse_str(cls: Type[ConfigObjectT], value: str) -> ConfigObjectT:
    """Custom implementation for parsing config values that are
    not handled by the default .parse(...) method."""
    raise NotImplementedError()

  @classmethod
  def is_valid_path(cls, path: pth.LocalPath) -> bool:
    if not path.is_file():
      return False
    return path.suffix in cls.VALID_EXTENSIONS

  @classmethod
  def parse_unknown_path(cls: Type[ConfigObjectT], path: pth.LocalPath,
                         **kwargs) -> ConfigObjectT:
    # TODO: this should be redirected to parse_config_path
    return cls.parse_str(str(path), **kwargs)

  @classmethod
  def parse_path(cls: Type[ConfigObjectT], path: pth.LocalPath,
                 **kwargs) -> ConfigObjectT:
    return cls.parse_config_path(path, **kwargs)

  @classmethod
  def parse_inline_hjson(cls: Type[ConfigObjectT], value: str,
                         **kwargs) -> ConfigObjectT:
    with exception.annotate(f"Parsing inline {cls.__name__}"):
      data = cli_helper.parse_inline_hjson(value)
      return cls.parse_dict(data, **kwargs)

  @classmethod
  def parse_config_path(cls: Type[ConfigObjectT], path: pth.LocalPathLike,
                        **kwargs) -> ConfigObjectT:
    with exception.annotate_argparsing(f"Parsing {cls.__name__} file: {path}"):
      file_path = cli_helper.parse_existing_file_path(path)
      data = cli_helper.parse_dict_hjson_file(file_path)
      with ChangeCWD(file_path.parent):
        return cls.parse_dict(data, **kwargs)

  @classmethod
  @abc.abstractmethod
  def parse_dict(cls: Type[ConfigObjectT], config: Dict[str,
                                                        Any]) -> ConfigObjectT:
    raise NotImplementedError()


class _ConfigKwargsParser:

  def __init__(self, parser: ConfigParser, config_data: Dict[str, Any]):
    self._parser = parser
    self._kwargs: Dict[str, Any] = {}
    self._processed_args: Set[str] = set()
    self._config_data = config_data
    self._parse()

  def _parse(self) -> None:
    for arg_parser in self._parser.arg_parsers:
      if arg_parser.name in self._processed_args:
        # Already previously handled by some depending_on argument.
        continue
      self._parse_arg(arg_parser)

  def _parse_arg(self, arg_parser: _ConfigArgParser) -> None:
    arg_name: str = arg_parser.name
    if arg_name in self._processed_args:
      raise ValueError(
          f"Recursive argument dependency on '{arg_name}' cannot be resolved.")
    self._processed_args.add(arg_name)
    with exception.annotate(f"Parsing ...['{arg_name}']:"):
      depending_kwargs = self._maybe_parse_depending_args(arg_parser)
      self._kwargs[arg_name] = arg_parser.parse(self._config_data,
                                                depending_kwargs)

  def _maybe_parse_depending_args(
      self, arg_parser: _ConfigArgParser) -> Dict[str, Any]:
    depending_args: Dict[str, Any] = {}
    if not arg_parser.depends_on:
      return depending_args
    with exception.annotate(f"Parsing ...['{arg_parser.name}'].depends_on:"):
      for depending_arg_name in arg_parser.depends_on:
        depending_args[depending_arg_name] = self._parse_depending_arg(
            depending_arg_name)
    return depending_args

  def _parse_depending_arg(self, arg_name: str) -> Any:
    if arg_name in self._kwargs:
      return self._kwargs[arg_name]
    with exception.annotate(f"Parsing ...['{arg_name}']:"):
      self._parse_arg(self._parser.get_argument(arg_name))
      assert arg_name in self._kwargs, (
          f"Failure when parsing depending {arg_name}")
    return self._kwargs[arg_name]

  def as_dict(self) -> Dict[str, Any]:
    return dict(self._kwargs)


ConfigResultObjectT = TypeVar("ConfigResultObjectT", bound="object")

class ConfigParser(Generic[ConfigResultObjectT]):

  def __init__(self,
               title: str,
               cls: Type[ConfigResultObjectT],
               default: Optional[ConfigResultObjectT] = None,
               allow_unused_config_data: bool = True) -> None:
    self.title = title
    assert title, "No title provided"
    self._cls = cls
    if default:
      if not isinstance(default, cls):
        raise TypeError(
            f"Default value '{default}' is not an instance of {cls.__name__}")
    self._default = default
    self._args: Dict[str, _ConfigArgParser] = {}
    self._arg_names: Set[str] = set()
    self._allow_unused_config_data = allow_unused_config_data

  @property
  def default(self) -> Optional[ConfigResultObjectT]:
    return self._default

  def add_argument(  # pylint: disable=redefined-builtin
      self,
      name: str,
      type: Optional[ArgParserType],
      default: Optional[Any] = None,
      choices: Optional[Iterable[Any]] = None,
      aliases: Tuple[str, ...] = tuple(),
      help: Optional[str] = None,
      is_list: bool = False,
      required: bool = False,
      depends_on: Optional[Iterable[str]] = None) -> None:
    if name in self._arg_names:
      raise ValueError(f"Duplicate argument: {name}")
    arg = self._args[name] = _ConfigArgParser(self, name, type, default,
                                              choices, aliases, help, is_list,
                                              required, depends_on)
    self._arg_names.add(name)
    for alias in arg.aliases:
      if alias in self._arg_names:
        raise ValueError(f"Argument alias ({alias}) from {name}"
                         " was previously added as argument.")
      self._arg_names.add(alias)

  def get_argument(self, arg_name: str) -> _ConfigArgParser:
    return self._args[arg_name]

  def kwargs_from_config(self, config_data: Dict[str, Any],
                         **extra_kwargs) -> Dict[str, Any]:
    with exception.annotate_argparsing(
        f"Parsing {self._cls.__name__} extra config kwargs:"):
      config_data = self._prepare_config_data(config_data, **extra_kwargs)
    with exception.annotate_argparsing(
        f"Parsing {self._cls.__name__} config dict:"):
      kwargs = _ConfigKwargsParser(self, config_data)
      if config_data:
        self._handle_unused_config_data(config_data)
      return kwargs.as_dict()

  def parse(self, config_data: Dict[str, Any], **kwargs) -> ConfigResultObjectT:
    if self._default and config_data == {} and not kwargs:
      return self._default
    kwargs = self.kwargs_from_config(config_data, **kwargs)
    return self.new_instance_from_kwargs(kwargs)

  def _prepare_config_data(self, config_data: Dict[str, Any],
                           **extra_kwargs) -> Dict[str, Any]:
    config_data = dict(config_data)
    for extra_key, extra_data in extra_kwargs.items():
      if extra_data is None:
        continue
      if extra_key in config_data and extra_data is not config_data[extra_key]:
        raise ValueError(
            f"Extra config data {repr(extra_key)}={repr(extra_data)} "
            f"was already present in config_data[..]={repr(config_data[extra_key])}"
        )
      config_data[extra_key] = extra_data
    return config_data

  def new_instance_from_kwargs(self, kwargs: Dict[str,
                                                  Any]) -> ConfigResultObjectT:
    return self._cls(**kwargs)

  def _handle_unused_config_data(self, unused_config_data: Dict[str,
                                                                Any]) -> None:
    logging.debug("Got unused properties: %s", unused_config_data.keys())
    if not self._allow_unused_config_data:
      unused_keys = ', '.join(map(repr, unused_config_data.keys()))
      raise argparse.ArgumentTypeError(
          f"Config for {self._cls.__name__} contains unused properties: "
          f"{unused_keys}")

  @property
  def arg_parsers(self) -> Tuple[_ConfigArgParser]:
    return tuple(self._args.values())

  @property
  def cls(self) -> Type:
    return self._cls

  @property
  def doc(self) -> str:
    if not self._cls.__doc__:
      return ""
    return self._cls.__doc__.strip()

  @property
  def help(self) -> str:
    return str(self)

  @property
  def summary(self) -> str:
    return self.doc.splitlines()[0]

  def __str__(self) -> str:
    parts: List[str] = []
    doc_string = self.doc
    width = 80
    if doc_string:
      parts.append("\n".join(textwrap.wrap(doc_string, width=width)))
      parts.append("")
    if not self._args:
      if parts:
        return parts[0]
      return ""
    parts.append(f"{self.title} Configuration:")
    parts.append("")
    for arg in self._args.values():
      parts.append(f"{arg.name}:")
      parts.extend(helper.wrap_lines(arg.help_text, width=width, indent="  "))
      parts.append("")
    return "\n".join(parts)
