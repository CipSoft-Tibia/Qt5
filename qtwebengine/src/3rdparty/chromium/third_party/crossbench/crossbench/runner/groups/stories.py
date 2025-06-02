# Copyright 2023 The Chromium Authors
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

from __future__ import annotations

from typing import TYPE_CHECKING, Iterable, List, Optional, Tuple

from crossbench import helper

from .base import RunGroup

if TYPE_CHECKING:
  from crossbench import exception
  from crossbench.browsers.browser import Browser
  from crossbench.probes.probe import Probe
  from crossbench.probes.results import ProbeResult
  from crossbench.runner.run import Run
  from crossbench.stories.story import Story
  from crossbench.types import JsonDict

  from .cache_temperature import CacheTemperatureRunGroup
  from .repetitions import RepetitionsRunGroup


class StoriesRunGroup(RunGroup):
  """
  A group of RepetitionsRunGroup for the same browser.
  """

  def __init__(self, throw: bool = False) -> None:
    super().__init__(throw)
    self._repetitions_groups: List[RepetitionsRunGroup] = []
    self._browser: Optional[Browser] = None

  @classmethod
  def groups(cls,
             run_groups: Iterable[RepetitionsRunGroup],
             throw: bool = False) -> Tuple[StoriesRunGroup, ...]:
    return tuple(
        helper.group_by(
            run_groups,
            key=lambda run_group: run_group.browser,
            group=lambda _: cls(throw),
            sort_key=None).values())

  def append(self, group: RepetitionsRunGroup) -> None:
    if self._path is None:
      self._set_path(group.path.parent)
      self._browser = group.browser
    assert self._path == group.path.parent
    assert self._browser == group.browser
    self._repetitions_groups.append(group)

  @property
  def repetitions_groups(self) -> List[RepetitionsRunGroup]:
    return self._repetitions_groups

  @property
  def cache_temperature_groups(self) -> Iterable[CacheTemperatureRunGroup]:
    for group in self._repetitions_groups:
      yield from group.cache_temperature_groups

  @property
  def runs(self) -> Iterable[Run]:
    for group in self._repetitions_groups:
      yield from group.runs

  @property
  def browser(self) -> Browser:
    assert self._browser
    return self._browser

  @property
  def stories(self) -> Iterable[Story]:
    return (group.story for group in self._repetitions_groups)

  @property
  def info_stack(self) -> exception.TInfoStack:
    return (
        "Merging results from multiple stories",
        f"browser={self.browser.unique_name}",
    )

  @property
  def info(self) -> JsonDict:
    info = {
        "label": self.browser.label,
        "browser": self.browser.app_name.title(),
        "version": self.browser.version,
        "os": self.browser.platform.full_version,
        "device": self.browser.platform.device,
        "cpu": self.browser.platform.cpu,
        "binary": str(self.browser.path),
        "flags": str(self.browser.flags),
        "runs": len(tuple(self.runs)),
        "failed runs": len(tuple(self.failed_runs))
    }
    info.update(super().info)
    return info

  def _merge_probe_results(self, probe: Probe) -> ProbeResult:
    # TODO: enable pytype again
    return probe.merge_stories(self)  # pytype: disable=wrong-arg-types
