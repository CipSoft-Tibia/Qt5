# Copyright 2023 The Chromium Authors
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

from __future__ import annotations

from typing import Tuple

from crossbench.benchmarks.jetstream.jetstream_2 import (JetStream2Benchmark,
                                                         JetStream2Probe,
                                                         JetStream2Story,
                                                         ProbeClsTupleT)


class JetStream20Probe(JetStream2Probe):
  __doc__ = JetStream2Probe.__doc__
  NAME: str = "jetstream_2.0"


class JetStream20Story(JetStream2Story):
  __doc__ = JetStream2Story.__doc__
  NAME: str = "jetstream_2.0"
  URL: str = "https://chromium-workloads.web.app/jetstream/v2.0/"
  URL_OFFICIAL: str = "https://browserbench.org/JetStream2.0/"


class JetStream20Benchmark(JetStream2Benchmark):
  """
  Benchmark runner for JetStream 2.0.
  """

  NAME: str = "jetstream_2.0"
  DEFAULT_STORY_CLS = JetStream20Story
  PROBES: ProbeClsTupleT = (JetStream20Probe,)

  @classmethod
  def version(cls) -> Tuple[int, ...]:
    return (2, 0)
