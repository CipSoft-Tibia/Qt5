# Copyright 2023 The Chromium Authors
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

from __future__ import annotations

from typing import TYPE_CHECKING, Dict, Iterable, List, Optional, Tuple

from crossbench.helper import collection_helper
from crossbench.path import LocalPath
from crossbench.runner.groups.base import RunGroup

if TYPE_CHECKING:
  from crossbench import exception
  from crossbench.browsers.browser import Browser
  from crossbench.probes.probe import Probe
  from crossbench.probes.results import ProbeResult
  from crossbench.runner.groups.cache_temperatures import \
      CacheTemperaturesRunGroup
  from crossbench.runner.run import Run
  from crossbench.stories.story import Story
  from crossbench.types import JsonDict, JsonMapping


class RepetitionsRunGroup(RunGroup):
  """
  A group of Run objects that are different repetitions for the same Story
  and the same browser, including all cache temperatures.
  """

  @classmethod
  def groups(cls,
             run_groups: Iterable[CacheTemperaturesRunGroup],
             throw: bool = False) -> Tuple[RepetitionsRunGroup, ...]:
    return tuple(
        collection_helper.group_by(
            run_groups,
            key=lambda group: (group.browser, group.story),
            group=lambda _: cls(throw),
            sort_key=None).values())

  def __init__(self, throw: bool = False):
    super().__init__(throw)
    self._cache_temperatures_groups: List[CacheTemperaturesRunGroup] = []
    self._cache_temperature_repetitions_groups: Dict[
        str, CacheTemperatureRepetitionsRunGroup] = {}
    self._story: Optional[Story] = None
    self._browser: Optional[Browser] = None

  def append(self, group: CacheTemperaturesRunGroup) -> None:
    if self._path is None:
      self._set_path(group.path.parent)
      self._story = group.story
      self._browser = group.browser
    assert self._story == group.story
    assert self._path == group.path.parent
    assert self._browser == group.browser
    self._cache_temperatures_groups.append(group)
    for run in group.runs:
      self._append_run(run)

  def _append_run(self, run: Run) -> None:
    temperature = run.temperature
    group = self._cache_temperature_repetitions_groups.get(temperature)
    if not group:
      group = CacheTemperatureRepetitionsRunGroup(self, self.throw)
      self._cache_temperature_repetitions_groups[temperature] = group
    group.append(run)

  @property
  def story(self) -> Story:
    assert self._story
    return self._story

  @property
  def browser(self) -> Browser:
    assert self._browser
    return self._browser

  @property
  def cache_temperatures_groups(self) -> List[CacheTemperaturesRunGroup]:
    return self._cache_temperatures_groups

  @property
  def cache_temperature_repetitions_groups(
      self) -> List[CacheTemperatureRepetitionsRunGroup]:
    return list(self._cache_temperature_repetitions_groups.values())

  @property
  def runs(self) -> Iterable[Run]:
    for group in self._cache_temperatures_groups:
      yield from group.runs

  @property
  def info_stack(self) -> exception.TInfoStack:
    return ("Merging results from multiple repetitions",
            f"browser={self.browser.unique_name}", f"story={self.story}")

  @property
  def info(self) -> JsonMapping:
    info: JsonDict = {"story": str(self.story)}
    info.update(super().info)
    return info

  def _merge_probe_results(self, probe: Probe) -> ProbeResult:
    return probe.merge_repetitions(self)


class CacheTemperatureRepetitionsRunGroup(RunGroup):
  """
  A group of Run objects that are different repetitions for the same Story
  and the same browser and the same cache temperatures.
  """

  def __init__(self,
               repetitions_group: RepetitionsRunGroup,
               throw: bool = False):
    super().__init__(throw)
    self._repetitions_group = repetitions_group
    self._set_path(repetitions_group.path)
    self._cache_temperature: str = ""
    self._runs: List[Run] = []

  @property
  def repetitions_group(self) -> RepetitionsRunGroup:
    return self._repetitions_group

  @property
  def story(self) -> Story:
    return self._repetitions_group.story

  @property
  def browser(self) -> Browser:
    return self._repetitions_group.browser

  @property
  def path(self) -> LocalPath:
    return self._repetitions_group.path

  @property
  def cache_temperature(self) -> str:
    return self._cache_temperature

  @property
  def runs(self) -> Iterable[Run]:
    return iter(self._runs)

  @property
  def info_stack(self) -> exception.TInfoStack:
    info_stack = self.repetitions_group.info_stack
    info_stack += (f"cache_temperature={self.cache_temperature}",)
    return info_stack

  @property
  def info(self) -> JsonMapping:
    info: JsonMapping = self._repetitions_group.info
    return info

  def append(self, run: Run) -> None:
    if not self._cache_temperature:
      self._cache_temperature = run.temperature
    assert self._cache_temperature == run.temperature
    self._runs.append(run)

  def _merge_probe_results(self, probe: Probe) -> ProbeResult:
    raise NotImplementedError("Unsupported")
