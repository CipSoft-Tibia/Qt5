# Copyright 2024 The Chromium Authors
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

from __future__ import annotations

from typing import Tuple

from crossbench.benchmarks.motionmark.motionmark_1 import (MotionMark1Benchmark,
                                                           MotionMark1Probe,
                                                           MotionMark1Story)


class MotionMark10Probe(MotionMark1Probe):
  __doc__ = MotionMark1Probe.__doc__
  NAME = "motionmark_1.0"


class MotionMark10Story(MotionMark1Story):
  NAME = "motionmark_1.0"
  URL: str = "https://chromium-workloads.web.app/motionmark/v1.0/"
  URL_OFFICIAL: str = "https://browserbench.org/MotionMark1.0/"


class MotionMark10Benchmark(MotionMark1Benchmark):
  """
  Benchmark runner for MotionMark 1.0.

  See https://browserbench.org/MotionMark1.0/ for more details.
  """

  NAME = "motionmark_1.0"
  DEFAULT_STORY_CLS = MotionMark10Story
  PROBES = (MotionMark10Probe,)

  @classmethod
  def version(cls) -> Tuple[int, ...]:
    return (1, 0)
