# Copyright 2023 The Chromium Authors
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

from __future__ import annotations

import argparse
import itertools
import logging
import pathlib
from typing import (TYPE_CHECKING, Any, Dict, Iterable, List, Optional, Set,
                    TextIO, Tuple, Type, Union, cast)

import hjson

import crossbench.browsers.all as browsers
from crossbench import cli_helper, exception, plt
from crossbench.browsers.browser_helper import (BROWSERS_CACHE,
                                                convert_flags_to_label)
from crossbench.browsers.chrome.downloader import ChromeDownloader
from crossbench.browsers.firefox.downloader import FirefoxDownloader
from crossbench.flags import ChromeFlags, Flags

from .base import ConfigError
from .browser import BrowserConfig
from .driver import BrowserDriverType

if TYPE_CHECKING:
  from crossbench.browsers.browser import Browser
  from crossbench.probes.probe import Probe
  FlagGroupItemT = Optional[Tuple[str, Optional[str]]]
  BrowserLookupTableT = Dict[str, Tuple[Type[Browser], "BrowserConfig"]]


def _map_flag_group_item(flag_name: str,
                         flag_value: Optional[str]) -> FlagGroupItemT:
  if flag_value is None:
    return None
  if flag_value == "":
    return (flag_name, None)
  return (flag_name, flag_value)


class FlagGroupConfig:
  """This object corresponds to a flag-group in a configuration file.
  It contains mappings from flags to multiple values.
  """
  _variants: Dict[str, Iterable[Optional[str]]]
  name: str

  def __init__(self, name: str,
               variants: Dict[str, Union[Iterable[Optional[str]], str]]):
    self.name = name
    self._variants = {}
    for flag_name, flag_variants_or_value in variants.items():
      assert flag_name not in self._variants
      assert flag_name
      if isinstance(flag_variants_or_value, str):
        self._variants[flag_name] = (str(flag_variants_or_value),)
      else:
        assert isinstance(flag_variants_or_value, Iterable)
        flag_variants = tuple(flag_variants_or_value)
        assert len(flag_variants) == len(set(flag_variants)), (
            "Flag variant contains duplicate entries: {flag_variants}")
        self._variants[flag_name] = tuple(flag_variants_or_value)

  def get_variant_items(self) -> Iterable[Tuple[FlagGroupItemT, ...]]:
    for flag_name, flag_values in self._variants.items():
      yield tuple(
          _map_flag_group_item(flag_name, flag_value)
          for flag_value in flag_values)


FlagItemT = Tuple[str, Optional[str]]


class BrowserVariantsConfig:

  @classmethod
  def from_cli_args(cls, args: argparse.Namespace) -> BrowserVariantsConfig:
    browser_config = BrowserVariantsConfig()
    if args.browser_config:
      with cli_helper.late_argument_type_error_wrapper("--browser-config"):
        path = args.browser_config.expanduser()
        with path.open(encoding="utf-8") as f:
          browser_config.load(f, args)
    else:
      with cli_helper.late_argument_type_error_wrapper("--browser"):
        browser_config.load_from_args(args)
    return browser_config

  def __init__(self,
               raw_config_data: Optional[Dict[str, Any]] = None,
               browser_lookup_override: Optional[BrowserLookupTableT] = None,
               args: Optional[argparse.Namespace] = None):
    self.flag_groups: Dict[str, FlagGroupConfig] = {}
    self._variants: List[Browser] = []
    self._unique_names: Set[str] = set()
    self._browser_lookup_override = browser_lookup_override or {}
    self._cache_dir: pathlib.Path = BROWSERS_CACHE
    if raw_config_data:
      assert args, "args object needed when loading from dict."
      self.load_dict(raw_config_data, args)

  @property
  def variants(self) -> List[Browser]:
    assert self._variants
    return self._variants

  def load(self, f: TextIO, args: argparse.Namespace) -> None:
    with exception.annotate(f"Loading browser config file: {f.name}"):
      config = {}
      with exception.annotate(f"Parsing {hjson.__name__}"):
        config = hjson.load(f)
      with exception.annotate(f"Parsing config file: {f.name}"):
        self.load_dict(config, args)

  def load_dict(self, config: Dict[str, Any], args: argparse.Namespace) -> None:
    with exception.annotate(
        f"Parsing {type(self).__name__} dict", throw_cls=ConfigError):
      if "flags" in config:
        with exception.annotate("Parsing config['flags']"):
          self._parse_flag_groups(config["flags"])
      if "browsers" not in config:
        raise ConfigError("Config does not provide a 'browsers' dict.")
      if not config["browsers"]:
        raise ConfigError("Config contains empty 'browsers' dict.")
      with exception.annotate("Parsing config['browsers']"):
        self._parse_browsers(config["browsers"], args)

  def load_from_args(self, args: argparse.Namespace) -> None:
    self._cache_dir = args.cache_dir
    browser_list: List[BrowserConfig] = args.browser or [
        BrowserConfig.default()
    ]
    assert isinstance(browser_list, list)
    if len(browser_list) != len(set(browser_list)):
      raise argparse.ArgumentTypeError(
          f"Got duplicate --browser arguments: {browser_list}")
    for i, browser in enumerate(browser_list):
      with exception.annotate(f"Append browser {i}"):
        self._append_browser(args, browser)
    self._verify_browser_flags(args)
    self._ensure_unique_browser_names()

  def _parse_flag_groups(self, data: Dict[str, Any]) -> None:
    for flag_name, group_config in data.items():
      with exception.annotate(f"Parsing flag-group: flags['{flag_name}']"):
        self._parse_flag_group(flag_name, group_config)

  def _parse_flag_group(self, name: str,
                        raw_flag_group_data: Dict[str, Any]) -> None:
    if name in self.flag_groups:
      raise ConfigError(f"flag-group flags['{name}'] exists already")
    variants: Dict[str, List[str]] = {}
    for flag_name, values in raw_flag_group_data.items():
      if not flag_name.startswith("-"):
        raise ConfigError(f"Invalid flag name: '{flag_name}'")
      if flag_name not in variants:
        flag_values = variants[flag_name] = []
      else:
        flag_values = variants[flag_name]
      if isinstance(values, str):
        values = [values]
      for value in values:
        if value == "None,":
          raise ConfigError(
              f"Please use null instead of None for flag '{flag_name}' ")
        # O(n^2) check, assuming very few values per flag.
        if value in flag_values:
          raise ConfigError("Same flag variant was specified more than once: "
                            f"'{value}' for entry '{flag_name}'")
        flag_values.append(value)
    self.flag_groups[name] = FlagGroupConfig(name, variants)

  def _parse_browsers(self, data: Dict[str, Any],
                      args: argparse.Namespace) -> None:
    for name, browser_config in data.items():
      with exception.annotate(f"Parsing browsers['{name}']"):
        self._parse_browser(name, browser_config, args)
    self._ensure_unique_browser_names()

  def _parse_browser(self, name: str, raw_browser_data: Dict[str, Any],
                     args: argparse.Namespace) -> None:
    path_or_identifier: Optional[str] = raw_browser_data.get("path")
    browser_cls: Type[Browser]
    if path_or_identifier in self._browser_lookup_override:
      browser_cls, browser_config = self._browser_lookup_override[
          path_or_identifier]
    else:
      browser_config = self._maybe_downloaded_binary(
          cast(BrowserConfig, BrowserConfig.parse(raw_browser_data)))
      browser_cls = self._get_browser_cls(browser_config)
    if browser_config.driver.type != BrowserDriverType.ANDROID and (
        not browser_config.path.exists()):
      raise ConfigError(
          f"browsers['{name}'].path='{browser_config.path}' does not exist.")
    raw_flags: List[Tuple[FlagItemT, ...]] = []
    with exception.annotate(f"Parsing browsers['{name}'].flags"):
      raw_flags = self._parse_flags(name, raw_browser_data)
    variants_flags: Tuple[Flags, ...] = ()
    with exception.annotate(
        f"Expand browsers['{name}'].flags into full variants"):
      variants_flags = tuple(
          browser_cls.default_flags(flags) for flags in raw_flags)
    logging.info("SELECTED BROWSER: '%s' with %s flag variants:", name,
                 len(variants_flags))
    for i in range(len(variants_flags)):
      logging.info("   %s: %s", i, variants_flags[i])
    browser_platform = self._get_browser_platform(browser_config)
    for flags in variants_flags:
      label = raw_browser_data.get("label", name)
      if len(variants_flags) > 1:
        label = self._flags_to_label(name, flags)
      if not self._check_unique_label(label):
        raise ConfigError(f"browsers['{name}'] has non-unique label: {label}")
      # pytype: disable=not-instantiable
      browser_instance = browser_cls(
          label=label,
          path=browser_config.path,
          flags=flags,
          driver_path=args.driver_path or browser_config.driver.path,
          # TODO: support all args in the browser.config file
          viewport=args.viewport,
          splash_screen=args.splash_screen,
          platform=browser_platform)
      # pytype: enable=not-instantiable
      self._variants.append(browser_instance)

  def _flags_to_label(self, name: str, flags: Flags) -> str:
    return f"{name}_{convert_flags_to_label(*flags.get_list())}"

  def _check_unique_label(self, label: str) -> bool:
    if label in self._unique_names:
      return False
    self._unique_names.add(label)
    return True

  def _parse_flags(self, name: str,
                   data: Dict[str, Any]) -> List[Tuple[FlagItemT, ...]]:
    flags_variants: List[Tuple[FlagGroupItemT, ...]] = []
    flag_group_names = data.get("flags", [])
    if isinstance(flag_group_names, str):
      flag_group_names = [flag_group_names]
    if not isinstance(flag_group_names, list):
      raise ConfigError(f"'flags' is not a list for browser='{name}'")
    seen_flag_group_names = set()
    for flag_group_name in flag_group_names:
      if flag_group_name in seen_flag_group_names:
        raise ConfigError(
            f"Duplicate group name '{flag_group_name}' for browser='{name}'")
      seen_flag_group_names.add(flag_group_name)
      # Use temporary FlagGroupConfig for inline fixed flag definition
      if flag_group_name.startswith("--"):
        flag_name, flag_value = Flags.split(flag_group_name)
        # No-value-flags produce flag_value == None, convert this to the "" for
        # compatibility with the flag variants, where None would mean removing
        # the flag.
        if flag_value is None:
          flag_value = ""
        flag_group = FlagGroupConfig("temporary", {flag_name: flag_value})
        assert flag_group_name not in self.flag_groups
      else:
        maybe_flag_group = self.flag_groups.get(flag_group_name, None)
        if maybe_flag_group is None:
          raise ConfigError(f"group='{flag_group_name}' "
                            f"for browser='{name}' does not exist.\n"
                            f"Choices are: {list(self.flag_groups.keys())}")
        flag_group = maybe_flag_group
      flags_variants += flag_group.get_variant_items()
    if len(flags_variants) == 0:
      # use empty default
      return [tuple()]
    # IN:  [
    #   (None,            ("--foo", "f1")),
    #   (("--bar", "b1"), ("--bar", "b2")),
    # ]
    # OUT: [
    #   (None,            ("--bar", "b1")),
    #   (None,            ("--bar", "b2")),
    #   (("--foo", "f1"), ("--bar", "b1")),
    #   (("--foo", "f1"), ("--bar", "b2")),
    # ]:
    flags_variants_combinations = list(itertools.product(*flags_variants))
    # IN: [
    #   (None,            None)
    #   (None,            ("--foo", "f1")),
    #   (("--foo", "f1"), ("--bar", "b1")),
    # ]
    # OUT: [
    #   (("--foo", "f1"),),
    #   (("--foo", "f1"), ("--bar", "b1")),
    # ]
    #
    flags_variants_filtered = list(
        tuple(flag_item
              for flag_item in flags_items
              if flag_item is not None)
        for flags_items in flags_variants_combinations)
    assert flags_variants_filtered
    return flags_variants_filtered

  def _get_browser_cls(self, browser_config: BrowserConfig) -> Type[Browser]:
    driver = browser_config.driver.type
    path = browser_config.path
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
    if "chromium" in path_str:
      # TODO: technically this should be ChromiumWebDriver
      if driver == BrowserDriverType.WEB_DRIVER:
        return browsers.ChromeWebDriver
      if driver == BrowserDriverType.APPLE_SCRIPT:
        return browsers.ChromeAppleScript
      if driver == BrowserDriverType.ANDROID:
        return browsers.ChromiumWebDriverAndroid
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
      browser.unique_name = f"{browser.type}_{browser.version}_{browser.label}"
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

  def _extract_chrome_flags(self, args: argparse.Namespace) -> ChromeFlags:
    flags = ChromeFlags()
    if args.enable_features:
      flags["--enable-features"] = args.enable_features
    if args.disable_features:
      flags["--disable-features"] = args.disable_features

    if args.js_flags:
      for js_flag in args.js_flags.split(","):
        js_flag_name, js_flag_value = Flags.split(js_flag.lstrip())
        flags.js_flags.set(js_flag_name, js_flag_value)

    if args.enable_field_trial_config is True:
      flags.set("--enable-field-trial-config")
    if args.enable_field_trial_config is False:
      flags.set("--disable-field-trial-config")
    return flags

  def _verify_browser_flags(self, args: argparse.Namespace) -> None:
    chrome_flags = self._extract_chrome_flags(args)
    for flag_name, value in chrome_flags.items():
      if not value:
        continue
      for browser in self._variants:
        if not isinstance(browser, browsers.Chromium):
          raise argparse.ArgumentTypeError(
              f"Used chrome/chromium-specific flags {flag_name} "
              f"for non-chrome {browser.unique_name}.\n"
              "Use --browser-config for complex variants.")
    browser_types = set(browser.type for browser in self._variants)
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
    if browser_config.driver.type == BrowserDriverType.ANDROID:
      return browser_config
    path_or_identifier = browser_config.browser
    if isinstance(path_or_identifier, pathlib.Path):
      return browser_config
    platform = plt.PLATFORM
    if ChromeDownloader.is_valid(path_or_identifier, platform):
      downloaded = ChromeDownloader.load(
          path_or_identifier, platform, cache_dir=self._cache_dir)
    elif FirefoxDownloader.is_valid(path_or_identifier, platform):
      downloaded = FirefoxDownloader.load(
          path_or_identifier, platform, cache_dir=self._cache_dir)
    else:
      raise ValueError(
          f"No version-download support for browser: {path_or_identifier}")
    return BrowserConfig(downloaded, browser_config.driver)

  def _append_browser(self, args: argparse.Namespace,
                      browser_config: BrowserConfig) -> None:
    assert browser_config, "Expected non-empty BrowserConfig."
    browser_config = self._maybe_downloaded_binary(browser_config)
    browser_cls: Type[Browser] = self._get_browser_cls(browser_config)
    path: pathlib.Path = browser_config.path
    flags = browser_cls.default_flags()

    if browser_config.driver.type != BrowserDriverType.ANDROID and (
        not path.exists()):
      raise argparse.ArgumentTypeError(f"Browser binary does not exist: {path}")

    if issubclass(browser_cls, browsers.Chromium):
      assert isinstance(flags, ChromeFlags)
      extra_flags = self._extract_chrome_flags(args)
      flags.merge(extra_flags)

    for flag_str in args.other_browser_args:
      flag_name, flag_value = Flags.split(flag_str)
      flags.set(flag_name, flag_value)

    browser_platform = self._get_browser_platform(browser_config)

    # Ignore the flags for the label since --browser only has a single
    # flag variant.
    label = f"{browser_platform}_{len(self._unique_names)}"
    assert self._check_unique_label(label), f"Non-unique label: {label}"

    browser_instance = browser_cls(  # pytype: disable=not-instantiable
        label=label,
        path=path,
        flags=flags,
        driver_path=args.driver_path or browser_config.driver.path,
        viewport=args.viewport,
        splash_screen=args.splash_screen,
        platform=browser_platform)
    logging.info("SELECTED BROWSER: name=%s path='%s' ",
                 browser_instance.unique_name, path)
    self._variants.append(browser_instance)
