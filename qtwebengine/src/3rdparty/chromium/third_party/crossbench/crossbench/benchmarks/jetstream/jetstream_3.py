# Copyright 2024 The Chromium Authors
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

from __future__ import annotations

import abc
from typing import Tuple, Type

from crossbench.benchmarks.jetstream.jetstream_2 import (JetStream2Benchmark,
                                                         JetStream2Probe,
                                                         JetStream2Story)


# TODO: introduce JetStreamProbe
class JetStream3Probe(JetStream2Probe, metaclass=abc.ABCMeta):
  """
  JetStream3-specific Probe.
  Extracts all JetStream 3 times and scores.
  """


# TODO: introduce JetStreamStory
class JetStream3Story(JetStream2Story, metaclass=abc.ABCMeta):
  SUBSTORIES: Tuple[str, ...] = ()


ProbeClsTupleT = Tuple[Type[JetStream3Probe], ...]


# TODO: introduce JetStreamBenchmark
class JetStream3Benchmark(JetStream2Benchmark):
  pass
