# Copyright 2023 The Chromium Authors
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

from __future__ import annotations

import argparse
import contextlib
import logging
from typing import (TYPE_CHECKING, Any, Dict, Iterator, List, Optional, Set,
                    TextIO, Tuple, Type, Union, cast)

import hjson

import crossbench.browsers.all as browsers
from crossbench import exception
from crossbench import path as pth
from crossbench import plt
from crossbench.browsers.browser_helper import convert_flags_to_label
from crossbench.browsers.chrome.downloader import ChromeDownloader
from crossbench.browsers.firefox.downloader import FirefoxDownloader
from crossbench.browsers.settings import Settings
from crossbench.cli.config.browser import BrowserConfig
from crossbench.cli.config.driver_type import BrowserDriverType
from crossbench.cli.config.flags import (DEFAULT_LABEL, FlagsConfig,
                                         FlagsGroupConfig, FlagsVariantConfig)
from crossbench.cli.config.network import NetworkConfig
from crossbench.config import ConfigError
from crossbench.flags.base import Flags
from crossbench.flags.chrome import ChromeFlags
from crossbench.helper.cwd import ChangeCWD
from crossbench.network.base import Network
from crossbench.parse import LateArgumentError, ObjectParser

if TYPE_CHECKING:
  from crossbench.browsers.browser import Browser
  FlagGroupItemT = Optional[Tuple[str, Optional[str]]]
  BrowserLookupTableT = Dict[str, Tuple[Type[Browser], "BrowserConfig"]]


@contextlib.contextmanager
def late_argument_type_error_wrapper(flag: str) -> Iterator[None]:
  """Converts raised ValueError and ArgumentTypeError to LateArgumentError
  that are associated with the given flag.
  """
  try:
    yield
  except Exception as e:
    raise LateArgumentError(flag, str(e)) from e


def _flags_to_label(flags: Flags) -> str:
  return convert_flags_to_label(*flags)


class BrowserVariantsConfig:

  @classmethod
  def from_cli_args(cls, args: argparse.Namespace) -> BrowserVariantsConfig:
    browser_config = BrowserVariantsConfig()
    if args.browser_config:
      with late_argument_type_error_wrapper("--browser-config"):
        path = args.browser_config.expanduser().absolute()
        with ChangeCWD(path.parent):
          with path.open(encoding="utf-8") as f:
            browser_config.parse_text_io(f, args)
    else:
      with late_argument_type_error_wrapper("--browser"):
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
      with exception.annotate("Parsing hjson"):
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
    browser_list: List[BrowserConfig] = args.browser or [
        BrowserConfig.default()
    ]
    assert isinstance(browser_list, list)
    browser_list = ObjectParser.unique_sequence(browser_list,
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
      browser_cls = self.get_browser_cls(browser_config)
    if not self._is_valid_browser_path(browser_config):
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
          driver_path=self._driver_path(args, browser_config),
          # TODO: support all args in the browser.config file
          viewport=args.viewport,
          splash_screen=args.splash_screen,
          platform=browser_platform,
          secrets=args.secrets,
          driver_logging=args.driver_logging,
          wipe_system_user_data=args.wipe_system_user_data,
          http_request_timeout=args.http_request_timeout)
      browser_instance = browser_cls(
          label=label, path=browser_config.path, settings=settings)
      # pytype: enable=not-instantiable
      self._variants.append(browser_instance)

  def _is_valid_browser_path(self, browser_config: BrowserConfig) -> bool:
    if browser_config.is_remote:
      # TODO: add remote path validation
      return True
    return pth.LocalPath(browser_config.path).exists()

  def _flags_to_label(self, name: str, flags: Flags) -> str:
    return f"{name}_{convert_flags_to_label(*flags)}"

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

  @classmethod
  def get_browser_cls(cls, browser_config: BrowserConfig) -> Type[Browser]:
    driver = browser_config.driver.type
    path: pth.AnyPath = browser_config.path
    assert not isinstance(path, str), "Invalid path"
    if not BrowserConfig.is_supported_browser_path(path):
      raise argparse.ArgumentTypeError(f"Unsupported browser path='{path}'")
    path_str = str(browser_config.path).lower()
    if "safari" in path_str:
      return cls.get_safari_browser_cls(browser_config)
    if "chrome" in path_str:
      return cls.get_chrome_browser_cls(browser_config)
    if "chromium" in path_str:
      return cls.get_chromium_browser_cls(browser_config)
    if "firefox" in path_str:
      if driver == BrowserDriverType.WEB_DRIVER:
        return browsers.FirefoxWebDriver
    if "edge" in path_str:
      return browsers.EdgeWebDriver
    raise argparse.ArgumentTypeError(f"Unsupported browser path='{path}'")

  @classmethod
  def get_safari_browser_cls(cls,
                             browser_config: BrowserConfig) -> Type[Browser]:
    driver = browser_config.driver.type
    if driver == BrowserDriverType.IOS:
      return browsers.SafariWebdriverIOS
    if driver == BrowserDriverType.WEB_DRIVER:
      return browsers.SafariWebDriver
    if driver == BrowserDriverType.APPLE_SCRIPT:
      return browsers.SafariAppleScript
    raise argparse.ArgumentTypeError(f"Unsupported Safari driver: {driver}")

  @classmethod
  def get_chrome_browser_cls(cls,
                             browser_config: BrowserConfig) -> Type[Browser]:
    driver = browser_config.driver.type
    if driver == BrowserDriverType.WEB_DRIVER:
      return browsers.ChromeWebDriver
    if driver == BrowserDriverType.APPLE_SCRIPT:
      return browsers.ChromeAppleScript
    if driver == BrowserDriverType.ANDROID:
      if browsers.LocalChromeWebDriverAndroid.is_apk_helper(
          browser_config.path):
        return browsers.LocalChromeWebDriverAndroid
      return browsers.ChromeWebDriverAndroid
    if driver == BrowserDriverType.LINUX_SSH:
      return browsers.ChromeWebDriverSsh
    if driver == BrowserDriverType.CHROMEOS_SSH:
      return browsers.ChromeWebDriverChromeOsSsh
    raise argparse.ArgumentTypeError(f"Unsupported Chrome driver: {driver}")

  @classmethod
  def get_chromium_browser_cls(cls,
                               browser_config: BrowserConfig) -> Type[Browser]:
    driver = browser_config.driver.type
    # TODO: technically this should be ChromiumWebDriver
    if driver == BrowserDriverType.WEB_DRIVER:
      return browsers.ChromiumWebDriver
    if driver == BrowserDriverType.APPLE_SCRIPT:
      return browsers.ChromiumAppleScript
    if driver == BrowserDriverType.ANDROID:
      if browsers.LocalChromiumWebDriverAndroid.is_apk_helper(
          browser_config.path):
        return browsers.LocalChromiumWebDriverAndroid
      return browsers.ChromiumWebDriverAndroid
    if driver == BrowserDriverType.LINUX_SSH:
      return browsers.ChromiumWebDriverSsh
    if driver == BrowserDriverType.CHROMEOS_SSH:
      return browsers.ChromiumWebDriverChromeOsSsh
    raise argparse.ArgumentTypeError(f"Unsupported chromium driver: {driver}")

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
      if not js_flags_str.strip():
        assert not flags.js_flags
      else:
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
    if args.remote_driver_path:
      raise argparse.ArgumentTypeError(
          f"Cannot use custom --remote-driver-path='{args.remote_driver_path}' "
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
    if isinstance(path_or_identifier, pth.AnyPath):
      return browser_config
    browser_platform = self._get_browser_platform(browser_config)
    if ChromeDownloader.is_valid(path_or_identifier, browser_platform):
      downloaded = ChromeDownloader.load(path_or_identifier, browser_platform)
    elif FirefoxDownloader.is_valid(path_or_identifier, browser_platform):
      downloaded = FirefoxDownloader.load(path_or_identifier, browser_platform)
    else:
      raise ValueError(
          f"No version-download support for browser: {path_or_identifier}")
    return BrowserConfig(downloaded, browser_config.driver)

  def _driver_path(self, args: argparse.Namespace,
                   browser_config: BrowserConfig) -> Optional[pth.AnyPath]:
    if browser_config.driver.is_remote:
      return args.remote_driver_path or browser_config.driver.path
    return args.driver_path or browser_config.driver.path

  def _append_browser(self, args: argparse.Namespace,
                      browser_config: BrowserConfig) -> None:
    assert browser_config, "Expected non-empty BrowserConfig."
    browser_config = self._maybe_downloaded_binary(browser_config)
    browser_cls: Type[Browser] = self.get_browser_cls(browser_config)
    path: pth.AnyPath = browser_config.path
    flags_sets = [browser_cls.default_flags()]
    if not self._is_valid_browser_path(browser_config):
      raise argparse.ArgumentTypeError(f"Browser binary does not exist: {path}")

    if issubclass(browser_cls, browsers.ChromiumBased):
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
          driver_path=self._driver_path(args, browser_config),
          viewport=args.viewport,
          splash_screen=args.splash_screen,
          platform=browser_platform,
          secrets=args.secrets,
          driver_logging=args.driver_logging,
          wipe_system_user_data=args.wipe_system_user_data,
          http_request_timeout=args.http_request_timeout)

      browser_instance = browser_cls(  # pytype: disable=not-instantiable # pylint: disable=abstract-class-instantiated
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
