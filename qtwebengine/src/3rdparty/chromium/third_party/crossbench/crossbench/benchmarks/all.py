# Copyright 2022 The Chromium Authors
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.
# pylint: disable=unused-import

from __future__ import annotations

from crossbench.benchmarks.jetstream import (JetStream20Benchmark,
                                             JetStream21Benchmark,
                                             JetStream22Benchmark,
                                             JetStream30Benchmark)
from crossbench.benchmarks.loading.loading_benchmark import LoadingBenchmark
from crossbench.benchmarks.loading.loadline_presets import (
    LoadLinePhoneBenchmark, LoadLineTabletBenchmark)
from crossbench.benchmarks.manual import ManualBenchmark
from crossbench.benchmarks.memory.memory_benchmark import MemoryBenchmark
from crossbench.benchmarks.motionmark import (MotionMark10Benchmark,
                                              MotionMark11Benchmark,
                                              MotionMark12Benchmark,
                                              MotionMark13Benchmark)
from crossbench.benchmarks.speedometer import (Speedometer20Benchmark,
                                               Speedometer21Benchmark,
                                               Speedometer30Benchmark)
