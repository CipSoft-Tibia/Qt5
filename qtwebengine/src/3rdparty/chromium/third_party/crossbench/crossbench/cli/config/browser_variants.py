# Copyright 2023 The Chromium Authors
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

from __future__ import annotations

import argparse
import dataclasses
import functools
import logging
from typing import (TYPE_CHECKING, Any, Dict, Final, List, Optional, Sequence,
                    Set, TextIO, Tuple, Type, Union, cast)

import hjson
from immutabledict import immutabledict
from ordered_set import OrderedSet

import crossbench.browsers.all as browsers
from crossbench import cli_helper, exception
from crossbench import path as pth
from crossbench import plt
from crossbench.browsers.browser_helper import (BROWSERS_CACHE,
                                                convert_flags_to_label)
from crossbench.browsers.chrome.downloader import ChromeDownloader
from crossbench.browsers.firefox.downloader import FirefoxDownloader
from crossbench.browsers.settings import Settings
from crossbench.cli.config.browser import BrowserConfig
from crossbench.cli.config.driver import BrowserDriverType
from crossbench.cli.config.network import NetworkConfig
from crossbench.config import ConfigError, ConfigObject
from crossbench.flags.base import Flags
from crossbench.flags.chrome import ChromeFlags
from crossbench.flags.js_flags import JSFlags
from crossbench.network.base import Network

if TYPE_CHECKING:
  from crossbench.browsers.browser import Browser
  FlagGroupItemT = Optional[Tuple[str, Optional[str]]]
  BrowserLookupTableT = Dict[str, Tuple[Type[Browser], "BrowserConfig"]]


def _flags_to_label(flags: Flags) -> str:
  return convert_flags_to_label(*flags)


FlagItemT = Tuple[str, Optional[str]]
FlagVariantsDictT = Dict[str, List[str]]

DEFAULT_LABEL: Final[str] = "default"

@dataclasses.dataclass(frozen=True)
class FlagsVariantConfig:
  label: str
  index: int = 0
  flags: Flags = dataclasses.field(default_factory=lambda: Flags().freeze())

  @classmethod
  def parse(cls, name: str, index: int, data: Any):
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


try:
  FlagsGroupConfigTuple = tuple[FlagsVariantConfig, ...]
except:  # pylint: disable=bare-except
  # Python 3.8 fallback
  FlagsGroupConfigTuple = tuple


class FlagsGroupConfig(FlagsGroupConfigTuple):
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
          cli_helper.parse_unique_sequence(
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
          FlagsVariantConfig(_flags_to_label(flags), len(variants), flags))
    return FlagsGroupConfig(tuple(variants))

  @classmethod
  def parse_str(cls, value: str) -> FlagsGroupConfig:
    if not value.strip():
      return FlagsGroupConfig()
    variants = (FlagsVariantConfig.parse(DEFAULT_LABEL, 0, value),)
    return FlagsGroupConfig(variants)

  def product(self, *args: FlagsGroupConfig) -> FlagsGroupConfig:
    return functools.reduce(lambda a, b: a._product(b), args, self)

  def _product(self, other: FlagsGroupConfig) -> FlagsGroupConfig:
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


class BrowserVariantsConfig:

  @classmethod
  def from_cli_args(cls, args: argparse.Namespace) -> BrowserVariantsConfig:
    browser_config = BrowserVariantsConfig()
    if args.browser_config:
      with cli_helper.late_argument_type_error_wrapper("--browser-config"):
        path = args.browser_config.expanduser()
        with path.open(encoding="utf-8") as f:
          browser_config.parse_text_io(f, args)
    else:
      with cli_helper.late_argument_type_error_wrapper("--browser"):
        browser_config.parse_args(args)
    return browser_config

  def __init__(self,
               raw_config_data: Optional[Dict[str, Any]] = None,
               browser_lookup_override: Optional[BrowserLookupTableT] = None,
               args: Optional[argparse.Namespace] = None):
    self.flags_config: FlagsConfig = FlagsConfig()
    self._variants: List[Browser] = []
    self._unique_names: Set[str] = set()
    self._browser_lookup_override = browser_lookup_override or {}
    self._cache_dir: pth.LocalPath = BROWSERS_CACHE
    if raw_config_data:
      assert args, "args object needed when loading from dict."
      self.parse_dict(raw_config_data, args)

  @property
  def variants(self) -> List[Browser]:
    assert self._variants
    return self._variants

  def parse_text_io(self, f: TextIO, args: argparse.Namespace) -> None:
    with exception.annotate(f"Loading browser config file: {f.name}"):
      config = {}
      with exception.annotate(f"Parsing {hjson.__name__}"):
        config = hjson.load(f)
      with exception.annotate(f"Parsing config file: {f.name}"):
        self.parse_dict(config, args)

  def parse_dict(self, config: Dict[str, Any],
                 args: argparse.Namespace) -> None:
    with exception.annotate(
        f"Parsing {type(self).__name__} dict", throw_cls=ConfigError):
      if "flags" in config:
        with exception.annotate("Parsing config['flags']"):
          self.flags_config = FlagsConfig.parse(config["flags"])
      if "browsers" not in config:
        raise ConfigError("Config does not provide a 'browsers' dict.")
      if not config["browsers"]:
        raise ConfigError("Config contains empty 'browsers' dict.")
      with exception.annotate("Parsing config['browsers']"):
        self._parse_browsers(config["browsers"], args)

  def parse_args(self, args: argparse.Namespace) -> None:
    self._cache_dir = args.cache_dir
    browser_list: List[BrowserConfig] = args.browser or [
        BrowserConfig.default()
    ]
    assert isinstance(browser_list, list)
    browser_list = cli_helper.parse_unique_sequence(browser_list,
                                                    "--browser arguments")
    for i, browser in enumerate(browser_list):
      with exception.annotate(f"Append browser {i}"):
        self._append_browser(args, browser)
    self._verify_browser_flags(args)
    self._ensure_unique_browser_names()

  def _parse_browsers(self, data: Dict[str, Any],
                      args: argparse.Namespace) -> None:
    for name, browser_config in data.items():
      with exception.annotate(f"Parsing browsers[{repr(name)}]"):
        self._parse_browser(name, browser_config, args)
    self._ensure_unique_browser_names()

  def _parse_browser(self, name: str, raw_browser_data: Any,
                     args: argparse.Namespace) -> None:
    if isinstance(raw_browser_data, (dict, str)):
      return self._parse_browser_dict(name, raw_browser_data, args)
    raise argparse.ArgumentTypeError(
        f"Expected str or dict, got {type(raw_browser_data).__name__}: "
        f"{repr(raw_browser_data)}")

  def _parse_browser_dict(self, name: str,
                          raw_browser_data: Union[str, Dict[str, Any]],
                          args: argparse.Namespace) -> None:
    path_or_identifier: Optional[str] = None
    if isinstance(raw_browser_data, dict):
      path_or_identifier = raw_browser_data.get("path")
    else:
      path_or_identifier = raw_browser_data
    browser_cls: Type[Browser]
    if path_or_identifier and (path_or_identifier
                               in self._browser_lookup_override):
      browser_cls, browser_config = self._browser_lookup_override[
          path_or_identifier]
    else:
      browser_config = self._maybe_downloaded_binary(
          cast(BrowserConfig, BrowserConfig.parse(raw_browser_data)))
      browser_cls = self._get_browser_cls(browser_config)
    if not browser_config.driver.type.is_remote and (not pth.LocalPath(
        browser_config.path).exists()):
      raise ConfigError(
          f"browsers[{repr(name)}].path='{browser_config.path}' does not exist."
      )
    flag_variants: FlagsGroupConfig = self._get_browser_variants(
        name, raw_browser_data)
    self._log_browser_variants(name, flag_variants)
    browser_platform = self._get_browser_platform(browser_config)
    labels_lookup = self._create_unique_variant_labels(name, raw_browser_data,
                                                       flag_variants)
    for variant in flag_variants:
      label = labels_lookup[variant]
      browser_flags = browser_cls.default_flags(variant.flags)
      with exception.annotate_argparsing("Creating network config"):
        network_config = browser_config.network or args.network
        network = self._get_browser_network(network_config, browser_platform)
      # TODO: move the browser instantiation to a separate step and only
      # create BrowserConfig objects first.
      # pytype: disable=not-instantiable
      settings = Settings(
          flags=browser_flags,
          network=network,
          driver_path=args.driver_path or browser_config.driver.path,
          # TODO: support all args in the browser.config file
          viewport=args.viewport,
          splash_screen=args.splash_screen,
          platform=browser_platform,
          secrets=args.secrets)
      browser_instance = browser_cls(
          label=label, path=browser_config.path, settings=settings)
      # pytype: enable=not-instantiable
      self._variants.append(browser_instance)

  def _flags_to_label(self, name: str, flags: Flags) -> str:
    return f"{name}_{_flags_to_label(flags)}"

  def _create_unique_variant_labels(self, name: str,
                                    raw_browser_data: Union[str, Dict[str,
                                                                      Any]],
                                    flag_variants: FlagsGroupConfig) -> Dict:
    labels_lookup: Dict[FlagsVariantConfig, str] = {}
    group_labels = set(variant.label for variant in flag_variants)
    use_unique_variant_label = len(group_labels) == len(flag_variants)

    for variant in flag_variants:
      label = name
      if isinstance(raw_browser_data, dict):
        label = raw_browser_data.get("label", name)
      if len(flag_variants) > 1:
        if use_unique_variant_label:
          label = f"{name}_{variant.label}"
        else:
          # TODO: This case might not happen anymore
          label = self._flags_to_label(name, variant.flags)
      if not self._check_unique_label(label):
        raise ConfigError(f"browsers[{repr(name)}] has non-unique label: "
                          f"{repr(label)}")
      labels_lookup[variant] = label
    return labels_lookup

  def _check_unique_label(self, label: str) -> bool:
    if label in self._unique_names:
      return False
    self._unique_names.add(label)
    return True

  def _get_browser_variants(
      self, browser_name: str,
      raw_browser_data: Union[str, Dict[str, Any]]) -> FlagsGroupConfig:
    default_variant = FlagsVariantConfig(DEFAULT_LABEL)
    flag_variants = FlagsGroupConfig((default_variant,))
    if not isinstance(raw_browser_data, dict):
      return flag_variants
    flag_groups: List[FlagsGroupConfig] = []
    with exception.annotate(f"Parsing browsers[{repr(browser_name)}].flags"):
      flag_groups = self._parse_browser_flags(browser_name, raw_browser_data)
    with exception.annotate(
        f"Expand browsers[{repr(browser_name)}].flags into full variants"):
      flag_variants = flag_variants.product(*flag_groups)
    return flag_variants

  def _parse_browser_flags(self, browser_name: str,
                           data: Dict[str, Any]) -> List[FlagsGroupConfig]:
    flag_group_names = data.get("flags", [])
    if isinstance(flag_group_names, str):
      flag_group_names = [flag_group_names]
    self._validate_flags(browser_name, flag_group_names)
    inline_flags = Flags()
    flag_groups: List[FlagsGroupConfig] = []
    for flag_group_name in flag_group_names:
      if flag_group_name.startswith("--"):
        inline_flags.update(Flags.parse(flag_group_name))
      else:
        maybe_flag_group = self.flags_config.get(flag_group_name, None)
        if maybe_flag_group is None:
          raise ConfigError(
              f"group={repr(flag_group_name)} "
              f"for browser={repr(browser_name)} does not exist.\n"
              f"Choices are: {list(self.flags_config.keys())}")
        flag_groups.append(maybe_flag_group)
    if inline_flags:
      flag_data = {"inline": inline_flags}
      flag_groups.append(FlagsGroupConfig.parse_dict(flag_data))
    return flag_groups

  def _validate_flags(self, browser_name: str, flag_group_names: List[str]):
    if isinstance(flag_group_names, str):
      flag_group_names = [flag_group_names]
    if not isinstance(flag_group_names, list):
      raise ConfigError(
          f"'flags' is not a list for browser={repr(browser_name)}")
    seen_flag_group_names: Set[str] = set()
    for flag_group_name in flag_group_names:
      if flag_group_name in seen_flag_group_names:
        raise ConfigError(f"Duplicate group name {repr(flag_group_name)} "
                          f"for browser={repr(browser_name)}")

  def _log_browser_variants(self, name: str,
                            flag_variants: FlagsGroupConfig) -> None:
    logging.info("SELECTED BROWSER: '%s' with %s flag variants:", name,
                 len(flag_variants))
    for i, variant in enumerate(flag_variants):
      logging.info("   %s: %s", i, variant.flags)

  def _get_browser_cls(self, browser_config: BrowserConfig) -> Type[Browser]:
    driver = browser_config.driver.type
    path: pth.RemotePath = browser_config.path
    assert not isinstance(path, str), "Invalid path"
    if not BrowserConfig.is_supported_browser_path(path):
      raise argparse.ArgumentTypeError(f"Unsupported browser path='{path}'")
    path_str = str(browser_config.path).lower()
    if "safari" in path_str:
      if driver == BrowserDriverType.IOS:
        return browsers.SafariWebdriverIOS
      if driver == BrowserDriverType.WEB_DRIVER:
        return browsers.SafariWebDriver
      if driver == BrowserDriverType.APPLE_SCRIPT:
        return browsers.SafariAppleScript
    if "chrome" in path_str:
      if driver == BrowserDriverType.WEB_DRIVER:
        return browsers.ChromeWebDriver
      if driver == BrowserDriverType.APPLE_SCRIPT:
        return browsers.ChromeAppleScript
      if driver == BrowserDriverType.ANDROID:
        return browsers.ChromeWebDriverAndroid
      if driver == BrowserDriverType.LINUX_SSH:
        return browsers.ChromeWebDriverSsh
      if driver == BrowserDriverType.CHROMEOS_SSH:
        return browsers.ChromeWebDriverChromeOsSsh
    if "chromium" in path_str:
      # TODO: technically this should be ChromiumWebDriver
      if driver == BrowserDriverType.WEB_DRIVER:
        return browsers.ChromeWebDriver
      if driver == BrowserDriverType.APPLE_SCRIPT:
        return browsers.ChromeAppleScript
      if driver == BrowserDriverType.ANDROID:
        return browsers.ChromiumWebDriverAndroid
      if driver == BrowserDriverType.LINUX_SSH:
        return browsers.ChromiumWebDriverSsh
      if driver == BrowserDriverType.CHROMEOS_SSH:
        return browsers.ChromiumWebDriverChromeOsSsh
    if "firefox" in path_str:
      if driver == BrowserDriverType.WEB_DRIVER:
        return browsers.FirefoxWebDriver
    if "edge" in path_str:
      return browsers.EdgeWebDriver
    raise argparse.ArgumentTypeError(f"Unsupported browser path='{path}'")

  def _get_browser_platform(self,
                            browser_config: BrowserConfig) -> plt.Platform:
    return browser_config.get_platform()

  def _ensure_unique_browser_names(self) -> None:
    if self._has_unique_variant_names():
      return
    # Expand to full version names
    for browser in self._variants:
      browser.unique_name = (
          f"{browser.type_name}_{browser.version}_{browser.label}")
    if self._has_unique_variant_names():
      return
    logging.info("Got unique browser names and versions, "
                 "please use --browser-config for more meaningful names")
    # Last resort, add index
    for index, browser in enumerate(self._variants):
      browser.unique_name += f"_{index}"
    assert self._has_unique_variant_names()

  def _has_unique_variant_names(self) -> bool:
    names = [browser.unique_name for browser in self._variants]
    unique_names = set(names)
    return len(unique_names) == len(names)

  def _extract_chrome_flags(self,
                            args: argparse.Namespace) -> List[ChromeFlags]:
    initial_flags = ChromeFlags()

    if args.enable_features:
      initial_flags["--enable-features"] = args.enable_features
    if args.disable_features:
      initial_flags["--disable-features"] = args.disable_features
    if args.enable_field_trial_config is True:
      initial_flags.set("--enable-field-trial-config")
    if args.enable_field_trial_config is False:
      initial_flags.set("--disable-field-trial-config")

    flags_sets = [initial_flags]
    if not args.js_flags:
      return flags_sets

    def copy_and_set_js_flags(flags: ChromeFlags,
                              js_flags_str: str) -> ChromeFlags:
      flags = flags.copy()
      for js_flag in js_flags_str.split(","):
        js_flag_name, js_flag_value = Flags.split(js_flag.lstrip())
        flags.js_flags.set(js_flag_name, js_flag_value)
      return flags

    flags_sets = [
        copy_and_set_js_flags(flags, js_flags_str)
        for flags in flags_sets
        for js_flags_str in args.js_flags
    ]
    return flags_sets

  def _verify_browser_flags(self, args: argparse.Namespace) -> None:
    for chrome_flags in self._extract_chrome_flags(args):
      for flag_name, value in chrome_flags.items():
        if not value:
          continue
        for browser in self._variants:
          if not browser.attributes.is_chromium_based:
            raise argparse.ArgumentTypeError(
                f"Used chrome/chromium-specific flags {flag_name} "
                f"for non-chrome {browser.unique_name}.\n"
                "Use --browser-config for complex variants.")
    browser_types = set(browser.type_name for browser in self._variants)
    if len(browser_types) == 1:
      return
    if args.driver_path:
      raise argparse.ArgumentTypeError(
          f"Cannot use custom --driver-path='{args.driver_path}' "
          f"for multiple browser {browser_types}.")
    if args.other_browser_args:
      raise argparse.ArgumentTypeError(
          f"Multiple browser types {browser_types} "
          "cannot be used with common extra browser flags: "
          f"{args.other_browser_args}.\n"
          "Use --browser-config for complex variants.")

  def _maybe_downloaded_binary(self,
                               browser_config: BrowserConfig) -> BrowserConfig:
    path_or_identifier = browser_config.browser
    if isinstance(path_or_identifier, pth.RemotePath):
      return browser_config
    browser_platform = self._get_browser_platform(browser_config)
    if ChromeDownloader.is_valid(path_or_identifier, browser_platform):
      downloaded = ChromeDownloader.load(
          path_or_identifier, browser_platform, cache_dir=self._cache_dir)
    elif FirefoxDownloader.is_valid(path_or_identifier, browser_platform):
      downloaded = FirefoxDownloader.load(
          path_or_identifier, browser_platform, cache_dir=self._cache_dir)
    else:
      raise ValueError(
          f"No version-download support for browser: {path_or_identifier}")
    return BrowserConfig(downloaded, browser_config.driver)

  def _append_browser(self, args: argparse.Namespace,
                      browser_config: BrowserConfig) -> None:
    assert browser_config, "Expected non-empty BrowserConfig."
    browser_config = self._maybe_downloaded_binary(browser_config)
    browser_cls: Type[Browser] = self._get_browser_cls(browser_config)
    path: pth.RemotePath = browser_config.path
    flags_sets = [browser_cls.default_flags()]

    if browser_config.driver.is_local and not pth.LocalPath(path).exists():
      raise argparse.ArgumentTypeError(f"Browser binary does not exist: {path}")

    if issubclass(browser_cls, browsers.Chromium):
      assert all(isinstance(flags, ChromeFlags) for flags in flags_sets)

      extra_flag_sets = self._extract_chrome_flags(args)
      flags_sets = [
          flags.merge_copy(extra_flags)
          for flags in flags_sets
          for extra_flags in extra_flag_sets
      ]

    for flag_str in args.other_browser_args:
      flag_name, flag_value = Flags.split(flag_str)
      for flags in flags_sets:
        flags.set(flag_name, flag_value)

    browser_platform = self._get_browser_platform(browser_config)
    with exception.annotate_argparsing("Creating network config"):
      network_config = browser_config.network or args.network
      network = self._get_browser_network(network_config, browser_platform)

    name = f"{browser_platform}_{len(self._unique_names)}"
    for flags in flags_sets:
      label = name
      if len(flags_sets) > 1:
        label = self._flags_to_label(label, flags)
      assert self._check_unique_label(label), f"Non-unique label: {label}"
      settings = Settings(
          flags=flags,
          network=network,
          driver_path=args.driver_path or browser_config.driver.path,
          viewport=args.viewport,
          splash_screen=args.splash_screen,
          platform=browser_platform,
          secrets=args.secrets)
      browser_instance = browser_cls(  # pytype: disable=not-instantiable
          label=label,
          path=path,
          settings=settings)
      logging.info("SELECTED BROWSER: name=%s path='%s' ",
                   browser_instance.unique_name, path)
      self._variants.append(browser_instance)

  def _get_browser_network(self, network_config: Union[pth.LocalPath,
                                                       NetworkConfig],
                           browser_platform: plt.Platform) -> Network:
    if not isinstance(network_config, NetworkConfig):
      network_config = NetworkConfig.parse(network_config)
    return network_config.create(browser_platform)
