# Copyright 2024 The Chromium Authors
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

from __future__ import annotations

import json
from typing import TYPE_CHECKING, Any, Dict, Iterable, List, Type

from crossbench.probes.internal.base import (InternalJsonResultProbe,
                                             InternalJsonResultProbeContext)
from crossbench.probes.results import EmptyProbeResult

if TYPE_CHECKING:
  from crossbench.probes.results import ProbeResult, ProbeResultDict
  from crossbench.runner.actions import Actions
  from crossbench.runner.groups.browsers import BrowsersRunGroup
  from crossbench.runner.groups.repetitions import RepetitionsRunGroup
  from crossbench.runner.groups.stories import StoriesRunGroup
  from crossbench.types import Json


class ErrorsProbe(InternalJsonResultProbe):
  """
  Runner-internal meta-probe: Collects all errors from running stories and/or
  from merging probe data.
  """
  NAME = "cb.errors"

  def merge_repetitions(self, group: RepetitionsRunGroup) -> ProbeResult:
    return self._merge_group(group, (run.results for run in group.runs))

  def merge_stories(self, group: StoriesRunGroup) -> ProbeResult:
    return self._merge_group(
        group, (rep_group.results for rep_group in group.repetitions_groups))

  def merge_browsers(self, group: BrowsersRunGroup) -> ProbeResult:
    return self._merge_group(
        group, (story_group.results for story_group in group.story_groups))

  def _merge_group(self, group,
                   results_iter: Iterable[ProbeResultDict]) -> ProbeResult:
    merged_errors: List[Dict[str, Any]] = []

    for results in results_iter:
      result = results[self]
      if not result:
        continue
      source_file = result.json
      assert source_file.is_file()
      with source_file.open(encoding="utf-8") as f:
        repetition_errors = json.load(f)
        assert isinstance(repetition_errors, list)
        merged_errors.extend(repetition_errors)

    group_errors = group.exceptions.to_json()
    assert isinstance(group_errors, list)
    merged_errors.extend(group_errors)

    if not merged_errors:
      return EmptyProbeResult()
    return self.write_group_result(group, merged_errors, csv_formatter=None)

  def get_context_cls(self) -> Type[ErrorsProbeContext]:
    return ErrorsProbeContext


class ErrorsProbeContext(InternalJsonResultProbeContext):

  def to_json(self, actions: Actions) -> Json:
    return self.run.exceptions.to_json()
