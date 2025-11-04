# Copyright 2022 The Chromium Authors
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.
# pylint: disable=unused-import

from __future__ import annotations

from crossbench.benchmarks.experimental.power import PowerBenchmark
from crossbench.benchmarks.jetstream import (JetStream20Benchmark,
                                             JetStream21Benchmark,
                                             JetStream22Benchmark,
                                             JetStream30Benchmark)
from crossbench.benchmarks.loading.loading_benchmark import PageLoadBenchmark
from crossbench.benchmarks.loading.loading_benchmark_presets import (
    PageLoadPhoneBenchmark, PageLoadTabletBenchmark)
from crossbench.benchmarks.manual import ManualBenchmark
from crossbench.benchmarks.motionmark import (MotionMark10Benchmark,
                                              MotionMark11Benchmark,
                                              MotionMark12Benchmark,
                                              MotionMark13Benchmark)
from crossbench.benchmarks.speedometer import (Speedometer20Benchmark,
                                               Speedometer21Benchmark,
                                               Speedometer30Benchmark)
