# Copyright 2024 The Chromium Authors
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

from __future__ import annotations

import abc
import logging
from typing import Dict, Iterable, Iterator, Optional, Tuple

from ordered_set import OrderedSet

from crossbench import path as pth
from crossbench.flags.base import Flags, Freezable
from crossbench.flags.js_flags import JSFlags
from crossbench.flags.known_js_flags import KNOWN_JS_FLAGS


class ChromeFlags(Flags):
  """Specialized Flags for Chrome/Chromium-based browser.

  This has special treatment for --js-flags and the feature flags:
  --enable-features/--disable-features
  --enable-blink-features/--disable-blink-features
  """
  _JS_FLAG = "--js-flags"

  def __init__(self, initial_data: Flags.InitialDataType = None) -> None:
    self._features = ChromeFeatures()
    self._blink_features = ChromeBlinkFeatures()
    self._js_flags = JSFlags()
    super().__init__(initial_data)

  def freeze(self) -> ChromeFlags:
    super().freeze()
    self._js_flags.freeze()
    self._features.freeze()
    self._blink_features.freeze()
    return self

  def __getitem__(self, key):
    if key == self._JS_FLAG and self._js_flags:
      return self._js_flags
    if key == ChromeFeatures.ENABLE_FLAG and self._features.enabled:
      return self._features.enabled_str()
    if key == ChromeFeatures.DISABLE_FLAG and self._features.disabled:
      return self._features.disabled_str()
    if key == ChromeBlinkFeatures.ENABLE_FLAG and self._blink_features.enabled:
      return self._blink_features.enabled_str()
    if key == ChromeBlinkFeatures.DISABLE_FLAG and self._blink_features.disabled:
      return self._blink_features.disabled_str()
    return super().__getitem__(key)

  def _set(self,
           flag_name: str,
           flag_value: Optional[str] = None,
           override: bool = False) -> None:
    self.assert_not_frozen()
    # pylint: disable=signature-differs
    if flag_name == ChromeFeatures.ENABLE_FLAG:
      if flag_value is None:
        raise ValueError(f"{ChromeFeatures.ENABLE_FLAG} cannot be None")
      for feature in flag_value.split(","):
        self._features.enable(feature)
    elif flag_name == ChromeFeatures.DISABLE_FLAG:
      if flag_value is None:
        raise ValueError(f"{ChromeFeatures.DISABLE_FLAG} cannot be None")
      for feature in flag_value.split(","):
        self._features.disable(feature)
    elif flag_name == ChromeBlinkFeatures.ENABLE_FLAG:
      if flag_value is None:
        raise ValueError(f"{ChromeBlinkFeatures.ENABLE_FLAG} cannot be None")
      for feature in flag_value.split(","):
        self._blink_features.enable(feature)
    elif flag_name == ChromeBlinkFeatures.DISABLE_FLAG:
      if flag_value is None:
        raise ValueError(f"{ChromeBlinkFeatures.DISABLE_FLAG} cannot be None")
      for feature in flag_value.split(","):
        self._blink_features.disable(feature)
    elif flag_name == self._JS_FLAG:
      if flag_value is None:
        raise ValueError(f"{self._JS_FLAG} cannot be None")
      self._set_js_flag(flag_value, override)
    else:
      flag_value = self._verify_flag(flag_name, flag_value)
      super()._set(flag_name, flag_value, override)

  def _set_js_flag(self, raw_js_flags: str, override: bool) -> None:
    new_js_flags = JSFlags(self._js_flags)
    for js_flag_name, js_flag_value in JSFlags.parse(raw_js_flags).items():
      new_js_flags.set(js_flag_name, js_flag_value, override=override)
    self._js_flags.update(new_js_flags)

  def _verify_flag(self, name: str, value: Optional[str]) -> Optional[str]:
    if candidate := self._find_misspelled_flag(name):
      logging.error(
          "Potentially misspelled flag: '%s'. "
          "Did you mean to use %s ?", name, candidate)
    if candidate := self._find_js_flag(name):
      js_flags = JSFlags()
      js_flags.set(candidate, value)
      logging.error(
          "Got potential V8 flag that should be used as "
          "--js-flags=%s", js_flags)
    if name == "--user-data-dir":
      if not value or not value.strip():
        raise ValueError("--user-data-dir cannot be the empty string.")
      # TODO: support remote platforms
      expanded_dir = str(pth.LocalPath(value).expanduser())
      if expanded_dir != value:
        logging.warning(
            "Chrome Flags: auto-expanding --user-data-dir from '%s' to '%s'",
            value, expanded_dir)
      return expanded_dir
    return value

  def _find_misspelled_flag(self, name: str) -> Optional[str]:
    if name in ("--enable-feature", "--enabled-feature", "--enabled-features"):
      return "--enable-features"
    if name in ("--disable-feature", "--disabled-feature",
                "--disabled-features"):
      return "--disable-features"
    if name in ("--enable-blink-feature", "--enabled-blink-feature",
                "--enabled-blink-features"):
      return "--enable-blink-features"
    if name in ("--disable-blink-feature", "--disabled-blink-feature",
                "--disabled-blink-features"):
      return "--disable-blink-features"
    return None

  def _find_js_flag(self, name: str) -> Optional[str]:
    normalized_name = name
    if name.startswith("--no-"):
      normalized_name = f"--{name[5:]}"
    elif name.startswith("--no"):
      normalized_name = f"--{name[4:]}"
    if normalized_name in KNOWN_JS_FLAGS:
      return name
    return None

  @property
  def features(self) -> ChromeFeatures:
    return self._features

  @property
  def blink_features(self) -> ChromeBlinkFeatures:
    return self._blink_features

  @property
  def js_flags(self) -> JSFlags:
    return self._js_flags

  def merge(self, other: Flags.InitialDataType) -> None:
    if not isinstance(other, ChromeFlags):
      other = ChromeFlags(other)
    self.features.merge(other.features)
    self.blink_features.merge(other.blink_features)
    self.js_flags.merge(other.js_flags)
    for name, value in other.base_items():
      self.set(name, value)

  def base_items(self) -> Iterable[Tuple[str, Optional[str]]]:
    yield from super().items()

  def items(self) -> Iterable[Tuple[str, Optional[str]]]:
    yield from self.base_items()
    if self._js_flags:
      yield (self._JS_FLAG, str(self.js_flags))
    yield from self.features.items()
    yield from self.blink_features.items()

  def __bool__(self) -> bool:
    return bool(self.data) or bool(self._js_flags) or bool(
        self._features) or bool(self._blink_features)


class ChromeBaseFeatures(Freezable, abc.ABC):
  ENABLE_FLAG: str = ""
  DISABLE_FLAG: str = ""

  def __init__(self) -> None:
    super().__init__()
    self._enabled: Dict[str, Optional[str]] = {}
    self._disabled: OrderedSet[str] = OrderedSet()

  @property
  def is_empty(self) -> bool:
    return len(self._enabled) == 0 and len(self._disabled) == 0

  @property
  def enabled(self) -> Dict[str, Optional[str]]:
    return dict(self._enabled)

  @property
  def disabled(self) -> OrderedSet[str]:
    return OrderedSet(self._disabled)

  def _parse_feature(self, feature: str) -> Tuple[str, Optional[str]]:
    if not feature:
      raise ValueError("Cannot parse empty feature")
    if "," in feature:
      raise ValueError(f"{repr(feature)} contains multiple features. "
                       "Please split them first.")
    return self._parse_feature_parts(feature)

  @abc.abstractmethod
  def _parse_feature_parts(self, feature: str) -> Tuple[str, Optional[str]]:
    pass

  def enable(self, feature: str) -> None:
    name, value = self._parse_feature(feature)
    self._enable(name, value)

  def _enable(self, name: str, value: Optional[str]) -> None:
    self.assert_not_frozen()
    if name in self._disabled:
      raise ValueError(
          f"Cannot enable previously disabled feature={repr(name)}")
    if name in self._enabled:
      prev_value = self._enabled[name]
      if value != prev_value:
        raise ValueError("Cannot set conflicting values "
                         f"({repr(prev_value)}, vs. {repr(value)}) "
                         f"for the same feature={repr(name)}")
    else:
      self._enabled[name] = value

  def disable(self, feature: str) -> None:
    self.assert_not_frozen()
    name, _ = self._parse_feature(feature)
    if name in self._enabled:
      raise ValueError(
          f"Cannot disable previously enabled feature={repr(name)}")
    self._disabled.add(name)

  def update(self, other: ChromeBaseFeatures) -> None:
    if not isinstance(other, type(self)):
      raise TypeError(f"Cannot merge {type(self)} with {type(other)}")
    for disabled in other.disabled:
      self.disable(disabled)
    for name, value in other.enabled.items():
      self._enable(name, value)

  def merge(self, other: ChromeBaseFeatures) -> None:
    self.update(other)

  def items(self) -> Iterable[Tuple[str, str]]:
    if self._enabled:
      yield (self.ENABLE_FLAG, self.enabled_str())
    if self._disabled:
      yield (self.DISABLE_FLAG, self.disabled_str())

  def enabled_str(self) -> str:
    return ",".join(
        k if v is None else f"{k}{v}" for k, v in self._enabled.items())

  def disabled_str(self) -> str:
    return ",".join(self._disabled)

  def __iter__(self) -> Iterator[str]:
    for flag_name, features_str in self.items():
      yield f"{flag_name}={features_str}"

  def __bool__(self):
    return bool(self._enabled) or bool(self._disabled)

  def __str__(self) -> str:
    return " ".join(self)


class ChromeFeatures(ChromeBaseFeatures):
  """
  Chrome Features set, throws if features are enabled and disabled at the same
  time.
  Examples:
    --disable-features="MyFeature1"
    --enable-features="MyFeature1,MyFeature2"
    --enable-features="MyFeature1:k1/v1/k2/v2,MyFeature2"
    --enable-features="MyFeature3<Trial2:k1/v1/k2/v2"
  """

  ENABLE_FLAG: str = "--enable-features"
  DISABLE_FLAG: str = "--disable-features"

  def _parse_feature_parts(self, feature: str) -> Tuple[str, Optional[str]]:
    parts = feature.split("<")
    if len(parts) == 2:
      return (parts[0], "<" + parts[1])
    if len(parts) != 1:
      raise ValueError(f"Invalid number of feature parts: {repr(parts)}")
    parts = feature.split(":")
    if len(parts) == 2:
      return (parts[0], ":" + parts[1])
    if len(parts) != 1:
      raise ValueError(f"Invalid number of feature parts: {repr(parts)}")
    return (feature, None)


class ChromeBlinkFeatures(ChromeBaseFeatures):
  """
  Chrome Features set, throws if features are enabled and disabled at the same
  time.
  Examples:
    --disable-blink-features="MyFeature1"
    --enable-blink-features="MyFeature1,MyFeature2"
  """

  ENABLE_FLAG: str = "--enable-blink-features"
  DISABLE_FLAG: str = "--disable-blink-features"

  def _parse_feature_parts(self, feature: str) -> Tuple[str, Optional[str]]:
    if "<" in feature or ":" in feature:
      raise ValueError("blink features do not have params, "
                       f"but found param separator in {repr(feature)}")
    return (feature, None)
