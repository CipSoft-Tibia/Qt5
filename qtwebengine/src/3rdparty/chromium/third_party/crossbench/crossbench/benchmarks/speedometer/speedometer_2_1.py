# Copyright 2023 The Chromium Authors
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

from __future__ import annotations

from typing import Tuple

from crossbench.benchmarks.speedometer.speedometer import (ProbeClsTupleT,
                                                           SpeedometerBenchmark)
from crossbench.benchmarks.speedometer.speedometer_2 import (Speedometer2Probe,
                                                             Speedometer2Story)


class Speedometer21Probe(Speedometer2Probe):
  NAME: str = "speedometer_2.1"


class Speedometer21Story(Speedometer2Story):
  NAME: str = "speedometer_2.1"
  URL: str = "https://chromium-workloads.web.app/speedometer/v2.1/"
  URL_OFFICIAL: str = "https://browserbench.org/Speedometer2.1/"


class Speedometer21Benchmark(SpeedometerBenchmark):
  """
  Benchmark runner for Speedometer 2.1
  """
  NAME: str = "speedometer_2.1"
  DEFAULT_STORY_CLS = Speedometer21Story
  PROBES: ProbeClsTupleT = (Speedometer21Probe,)

  @classmethod
  def version(cls) -> Tuple[int, ...]:
    return (2, 1)

  @classmethod
  def aliases(cls) -> Tuple[str, ...]:
    return ("sp", "speedometer", "sp2", "speedometer2") + super().aliases()
