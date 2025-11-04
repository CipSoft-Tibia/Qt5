# Copyright 2024 The Chromium Authors
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

from __future__ import annotations

import abc
import contextlib
import logging
from typing import TYPE_CHECKING, Iterable, Iterator, Tuple

from crossbench import plt
from crossbench.helper import DurationMeasureContext, Durations
from crossbench.probes.result_location import ResultLocation

if TYPE_CHECKING:
  from crossbench.browsers.browser import Browser
  from crossbench.exception import (Annotator, ExceptionAnnotationScope,
                                    TExceptionTypes)
  from crossbench.path import LocalPath, RemotePath
  from crossbench.probes.probe import Probe
  from crossbench.runner.runner import Runner


class ResultOrigin(abc.ABC):
  """Base class for Run and BrowserSession, both places where
  probe results can be placed."""

  @property
  def is_local(self) -> bool:
    return self.browser_platform.is_local

  @property
  def is_remote(self) -> bool:
    return self.browser_platform.is_remote

  @property
  @abc.abstractmethod
  def browser_tmp_dir(self) -> RemotePath:
    pass

  @property
  @abc.abstractmethod
  def out_dir(self) -> LocalPath:
    pass

  @property
  @abc.abstractmethod
  def exceptions(self) -> Annotator:
    pass

  @property
  @abc.abstractmethod
  def durations(self) -> Durations:
    pass

  @property
  @abc.abstractmethod
  def browser(self) -> Browser:
    pass

  @property
  @abc.abstractmethod
  def runner(self) -> Runner:
    pass

  @property
  def runner_platform(self) -> plt.Platform:
    return self.runner.platform

  @property
  def browser_platform(self) -> plt.Platform:
    return self.browser.platform

  @property
  def probes(self) -> Iterable[Probe]:
    return self.runner.probes

  @contextlib.contextmanager
  def measure(
      self, label: str
  ) -> Iterator[Tuple[ExceptionAnnotationScope, DurationMeasureContext]]:
    # Return a combined context manager that adds an named exception info
    # and measures the time during the with-scope.
    with self.exceptions.info(label) as stack, self.durations.measure(
        label) as timer:
      yield (stack, timer)

  def exception_info(self, *stack_entries: str) -> ExceptionAnnotationScope:
    return self.exceptions.info(*stack_entries)

  def exception_handler(
      self, *stack_entries: str, exceptions: TExceptionTypes = (Exception,)
  ) -> ExceptionAnnotationScope:
    return self.exceptions.capture(*stack_entries, exceptions=exceptions)

  def get_default_probe_result_path(self, probe: Probe) -> RemotePath:
    """Return a local or remote/browser-based result path depending on the
    Probe default RESULT_LOCATION."""
    if probe.RESULT_LOCATION == ResultLocation.BROWSER:
      return self.get_browser_probe_result_path(probe)
    if probe.RESULT_LOCATION == ResultLocation.LOCAL:
      return self.get_local_probe_result_path(probe)
    raise ValueError(f"Invalid probe.RESULT_LOCATION {probe.RESULT_LOCATION} "
                     f"for probe {probe}")

  @abc.abstractmethod
  def get_local_probe_result_path(self, probe: Probe) -> LocalPath:
    pass

  def get_browser_probe_result_path(self, probe: Probe) -> RemotePath:
    local_path = self.get_local_probe_result_path(probe)
    if self.is_local:
      return local_path
    # Create a temp file relative to the remote browser tmp dir.
    relative_path = local_path.relative_to(self.out_dir)
    path = self.browser_tmp_dir / relative_path
    logging.debug("Creating remote result dir=%s on platform=%s", path.parent,
                  self.browser_platform)
    self.browser_platform.mkdir(path.parent)
    return path
