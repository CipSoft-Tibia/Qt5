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
  from crossbench.runner.runner import Runner
  from crossbench.types import JsonDict
  from crossbench.stories.story import Story


class CacheTemperatureRunGroup(RunGroup):
  """
  A group of Run objects with different cache temperatures for the same Story
  with same browser and same iteration.
  """

  @classmethod
  def groups(cls,
             runs: Iterable[Run],
             throw: bool = False) -> Tuple[CacheTemperatureRunGroup, ...]:
    return tuple(
        helper.group_by(
            runs,
            key=lambda run: (run.story, run.browser, run.repetition),
            group=lambda _: cls(throw),
            sort_key=None).values())

  def __init__(self, throw: bool = False):
    super().__init__(throw)
    self._runs: List[Run] = []
    self._story: Optional[Story] = None
    self._browser: Optional[Browser] = None
    self._repetition = -1

  def append(self, run: Run) -> None:
    if self._path is None:
      self._set_path(run.group_dir)
      self._story = run.story
      self._browser = run.browser
      self._repetition = run.repetition
    assert self._story == run.story
    assert self._path == run.group_dir
    assert self._browser == run.browser
    assert self._repetition == run.repetition
    self._runs.append(run)

  @property
  def runs(self) -> Iterable[Run]:
    return iter(self._runs)

  @property
  def repetition(self) -> int:
    return self._repetition

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
    return ("Merging results from multiple cache temperatures",
            f"browser={self.browser.unique_name}", f"story={self.story}",
            f"repetition={self.repetition}")

  @property
  def info(self) -> JsonDict:
    info = {
        "story": str(self.story),
        "repetition": self.repetition,
    }
    info.update(super().info)
    return info

  def _merge_probe_results(self, probe: Probe) -> ProbeResult:
    return probe.merge_cache_temperatures(self)  # pytype: disable=wrong-arg-types
