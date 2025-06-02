# Copyright 2023 The Chromium Authors
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

from __future__ import annotations

from typing import TYPE_CHECKING, Iterable, List, Optional, Tuple

from .base import RunGroup

from crossbench import helper

if TYPE_CHECKING:
  from crossbench import exception
  from crossbench.browsers.browser import Browser
  from crossbench.probes.probe import Probe
  from crossbench.probes.results import ProbeResult
  from crossbench.runner.run import Run
  from crossbench.runner.runner import Runner
  from crossbench.stories.story import Story
  from crossbench.types import JsonDict

  from .cache_temperature import CacheTemperatureRunGroup


class RepetitionsRunGroup(RunGroup):
  """
  A group of Run objects that are different repetitions for the same Story
  and the same browser.
  """

  @classmethod
  def groups(cls,
             run_groups: Iterable[CacheTemperatureRunGroup],
             throw: bool = False) -> Tuple[RepetitionsRunGroup, ...]:
    return tuple(
        helper.group_by(
            run_groups,
            key=lambda group: (group.browser, group.story),
            group=lambda _: cls(throw),
            sort_key=None).values())

  def __init__(self, throw: bool = False):
    super().__init__(throw)
    self._cache_temperature_groups: List[CacheTemperatureRunGroup] = []
    self._story: Optional[Story] = None
    self._browser: Optional[Browser] = None

  def append(self, group: CacheTemperatureRunGroup) -> None:
    if self._path is None:
      self._set_path(group.path.parent)
      self._story = group.story
      self._browser = group.browser
    assert self._story == group.story
    assert self._path == group.path.parent
    assert self._browser == group.browser
    self._cache_temperature_groups.append(group)

  @property
  def cache_temperature_groups(self) -> List[CacheTemperatureRunGroup]:
    return self._cache_temperature_groups

  @property
  def runs(self) -> Iterable[Run]:
    for group in self._cache_temperature_groups:
      yield from group.runs

  @property
  def story(self) -> Story:
    assert self._story
    return self._story

  @property
  def browser(self) -> Browser:
    assert self._browser
    return self._browser

  @property
  def info_stack(self) -> exception.TInfoStack:
    return ("Merging results from multiple repetitions",
            f"browser={self.browser.unique_name}", f"story={self.story}")

  @property
  def info(self) -> JsonDict:
    info = {"story": str(self.story)}
    info.update(super().info)
    return info

  def _merge_probe_results(self, probe: Probe) -> ProbeResult:
    return probe.merge_repetitions(self)  # pytype: disable=wrong-arg-types
