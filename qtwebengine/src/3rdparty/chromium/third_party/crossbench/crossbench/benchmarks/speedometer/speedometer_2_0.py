# Copyright 2023 The Chromium Authors
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

from __future__ import annotations

from typing import Tuple

from crossbench.benchmarks.speedometer.speedometer import (ProbeClsTupleT,
                                                           SpeedometerBenchmark)
from crossbench.benchmarks.speedometer.speedometer_2 import (Speedometer2Probe,
                                                             Speedometer2Story)


class Speedometer20Probe(Speedometer2Probe):
  NAME: str = "speedometer_2.0"


class Speedometer20Story(Speedometer2Story):
  NAME: str = "speedometer_2.0"
  URL: str = "https://chromium-workloads.web.app/speedometer/v2.0/"
  URL_OFFICIAL: str = "https://browserbench.org/Speedometer2.0/"


class Speedometer20Benchmark(SpeedometerBenchmark):
  """
  Benchmark runner for Speedometer 2.0
  """
  NAME: str = "speedometer_2.0"
  DEFAULT_STORY_CLS = Speedometer20Story
  PROBES: ProbeClsTupleT = (Speedometer20Probe,)

  @classmethod
  def version(cls) -> Tuple[int, ...]:
    return (2, 0)
