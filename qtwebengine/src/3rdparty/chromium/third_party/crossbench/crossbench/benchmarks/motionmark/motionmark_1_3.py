# Copyright 2024 The Chromium Authors
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

from __future__ import annotations

from typing import Tuple

from crossbench.benchmarks.motionmark.motionmark_1 import (MotionMark1Benchmark,
                                                           MotionMark1Probe,
                                                           MotionMark1Story)


class MotionMark13Probe(MotionMark1Probe):
  __doc__ = MotionMark1Probe.__doc__
  NAME = "motionmark_1.3"


class MotionMark13Story(MotionMark1Story):
  NAME = "motionmark_1.3"
  URL: str = "https://chromium-workloads.web.app/motionmark/v1.3/"
  URL_OFFICIAL: str = "https://browserbench.org/MotionMark1.3/"
  DEVELOPER_READY_JS: str = (
      "return !(document.querySelector('#frame-rate-detection span'));")
  READY_JS: str = (
      "return !!("
      "   document.querySelector('#frame-rate-label')?.textContent?.trim());")


class MotionMark13Benchmark(MotionMark1Benchmark):
  """
  Benchmark runner for MotionMark 1.3.

  See https://browserbench.org/MotionMark1.3/ for more details.
  """

  NAME = "motionmark_1.3"
  DEFAULT_STORY_CLS = MotionMark13Story
  PROBES = (MotionMark13Probe,)

  @classmethod
  def version(cls) -> Tuple[int, ...]:
    return (1, 3)
