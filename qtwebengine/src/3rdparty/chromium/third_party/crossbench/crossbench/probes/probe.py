# Copyright 2022 The Chromium Authors
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

from __future__ import annotations

import abc
import argparse
from typing import (TYPE_CHECKING, Any, Dict, Hashable, Optional, Set, Tuple,
                    Type, TypeVar)

from crossbench import plt
from crossbench.browsers.attributes import BrowserAttributes
from crossbench.config import ConfigParser
from crossbench.probes.probe_context import ProbeContext, ProbeSessionContext
from crossbench.probes.result_location import ResultLocation
from crossbench.probes.results import EmptyProbeResult, ProbeResult

if TYPE_CHECKING:
  from crossbench.browsers.browser import Browser
  from crossbench.env import HostEnvironment
  from crossbench.runner.groups import (BrowserSessionRunGroup,
                                        BrowsersRunGroup,
                                        CacheTemperatureRunGroup,
                                        RepetitionsRunGroup, StoriesRunGroup)
  from crossbench.runner.run import Run


ProbeT = TypeVar("ProbeT", bound="Probe")


class ProbeConfigParser(ConfigParser[ProbeT]):

  def __init__(self, probe_cls: Type[ProbeT]) -> None:
    super().__init__("Probe", probe_cls, allow_unused_config_data=False)
    self._probe_cls: Type[ProbeT] = probe_cls

  @property
  def probe_cls(self) -> Type[ProbeT]:
    return self._probe_cls


class ProbeMissingDataError(ValueError):
  pass


class ProbeValidationError(ValueError):

  def __init__(self, probe: Probe, message: str) -> None:
    self.probe = probe
    super().__init__(f"Probe({probe.NAME}): {message}")


class ProbeIncompatibleBrowser(ProbeValidationError):

  def __init__(self,
               probe: Probe,
               browser: Browser,
               message: str = "Incompatible browser") -> None:
    super().__init__(probe, f"{message}, got {browser.attributes}")


ProbeKeyT = Tuple[Tuple[str, Hashable], ...]


class Probe(abc.ABC):
  """
  Abstract Probe class.

  Probes are responsible for extracting performance numbers from websites
  / stories.

  Probe interface:
  - scope(): Return a custom ProbeContext (see below)
  - validate_browser(): Customize to display warnings before using Probes with
    incompatible settings / browsers.
  The Probe object can the customize how to merge probe (performance) date at
  multiple levels:
  - multiple repetitions of the same story
  - merged repetitions from multiple stories (same browser)
  - Probe data from all Runs

  Probes use a ProbeContext that is active during a story-Run.
  The ProbeContext class defines a customizable interface
  - setup(): Used for high-overhead Probe initialization
  - start(): Low-overhead start-to-measure signal
  - stop():  Low-overhead stop-to-measure signal
  - teardown(): Used for high-overhead Probe cleanup

  """
  NAME: str = ""

  @classmethod
  def config_parser(cls) -> ProbeConfigParser:
    return ProbeConfigParser(cls)

  @classmethod
  def from_config(cls: Type[ProbeT], config_data: Dict) -> ProbeT:
    return cls.config_parser().parse(config_data)

  @classmethod
  def help_text(cls) -> str:
    return cls.config_parser().help

  @classmethod
  def summary_text(cls) -> str:
    return cls.config_parser().summary

  # Set to False if the Probe cannot be used with arbitrary Stories or Pages
  IS_GENERAL_PURPOSE: bool = True
  PRODUCES_DATA: bool = True
  # Set the default probe result location, used to figure out whether result
  # files need to be transferred from a remote machine.
  RESULT_LOCATION = ResultLocation.LOCAL
  # Set to True if the probe only works on battery power with single runs
  BATTERY_ONLY: bool = False

  def __init__(self) -> None:
    assert self.name is not None, "A Probe must define a name"
    self._browsers: Set[Browser] = set()

  def __str__(self) -> str:
    return type(self).__name__

  def __eq__(self, other) -> bool:
    if self is other:
      return True
    if type(self) is not type(other):
      return False
    return self.key == other.key

  @property
  def is_internal(self) -> bool:
    """Returns True for subclasses of InternalProbe that are not
    directly user-accessible."""
    return False

  @property
  def key(self) -> ProbeKeyT:
    """Return a sort key."""
    return (("name", self.name),)

  def __hash__(self) -> int:
    return hash(self.key)

  @property
  def runner_platform(self) -> plt.Platform:
    return plt.PLATFORM

  @property
  def name(self) -> str:
    return self.NAME

  @property
  def result_path_name(self) -> str:
    return self.name

  @property
  def is_attached(self) -> bool:
    return len(self._browsers) > 0

  def attach(self, browser: Browser) -> None:
    assert browser not in self._browsers, (
        f"Probe={self.name} is attached multiple times to the same browser")
    self._browsers.add(browser)

  def validate_env(self, env: HostEnvironment) -> None:
    """
    Part of the Checklist, make sure everything is set up correctly for a probe
    to run.
    Browser-only validation is handled in validate_browser(...).
    """
    # Ensure that the proper super methods for setting up a probe were
    # called.
    assert self.is_attached, (
        f"Probe {self.name} is not properly attached to a browser")
    for browser in self._browsers:
      self.validate_browser(env, browser)

  def validate_browser(self, env: HostEnvironment, browser: Browser) -> None:
    """
    Validate that browser is compatible with this Probe.
    - Raise ProbeValidationError for hard-errors,
    - Use env.handle_warning for soft errors where we expect recoverable errors
      or only partially broken results.
    """
    del env, browser

  def expect_browser(self,
                     browser: Browser,
                     attributes: BrowserAttributes,
                     message: Optional[str] = None) -> None:
    if attributes in browser.attributes:
      return
    if not message:
      message = f"Incompatible browser, expected {attributes}"
    raise ProbeIncompatibleBrowser(self, browser, message)

  def expect_macos(self, browser: Browser) -> None:
    if not browser.platform.is_macos:
      raise ProbeIncompatibleBrowser(self, browser, "Only supported on macOS")

  def merge_cache_temperatures(self,
                               group: CacheTemperatureRunGroup) -> ProbeResult:
    """
    For merging probe data from multiple browser cache temperatures with the
    same repetition, story and browser.
    """
    # Return the first result by default.
    return tuple(group.runs)[0].results[self]

  def merge_repetitions(self, group: RepetitionsRunGroup) -> ProbeResult:
    """
    For merging probe data from multiple repetitions of the same story.
    """
    del group
    return EmptyProbeResult()

  def merge_stories(self, group: StoriesRunGroup) -> ProbeResult:
    """
    For merging multiple stories for the same browser.
    """
    del group
    return EmptyProbeResult()

  def merge_browsers(self, group: BrowsersRunGroup) -> ProbeResult:
    """
    For merging all probe data (from multiple stories and browsers.)
    """
    del group
    return EmptyProbeResult()

  @abc.abstractmethod
  def get_context(self: ProbeT, run: Run) -> Optional[ProbeContext[ProbeT]]:
    pass

  def get_session_context(
      self: ProbeT,
      session: BrowserSessionRunGroup) -> Optional[ProbeSessionContext[ProbeT]]:
    del session

  def log_run_result(self, run: Run) -> None:
    """
    Override to print a short summary of the collected results after a run
    completes.
    """
    del run

  def log_browsers_result(self, group: BrowsersRunGroup) -> None:
    """
    Override to print a short summary of all the collected results.
    """
    del group
