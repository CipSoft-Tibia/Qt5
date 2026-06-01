# Copyright 2024 The Chromium Authors
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

from __future__ import annotations

from typing import TYPE_CHECKING, Type

from crossbench.probes.internal.base import (InternalJsonResultProbe,
                                             InternalJsonResultProbeContext)
from crossbench.probes.results import EmptyProbeResult

if TYPE_CHECKING:
  from crossbench.probes.results import ProbeResult
  from crossbench.runner.actions import Actions
  from crossbench.runner.groups.repetitions import RepetitionsRunGroup
  from crossbench.types import Json


class SystemDetailsProbe(InternalJsonResultProbe):
  """
  Runner-internal meta-probe: Collects the browser's system/platform details.
  """
  NAME = "cb.system.details"

  def merge_repetitions(self, group: RepetitionsRunGroup) -> ProbeResult:
    return EmptyProbeResult()

  def get_context_cls(self) -> Type[InternalJsonResultProbeContext]:
    return SystemDetailsProbeContext


class SystemDetailsProbeContext(InternalJsonResultProbeContext):

  def to_json(self, actions: Actions) -> Json:
    return self.run.browser_platform.system_details()
