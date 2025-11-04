# Copyright 2022 The Chromium Authors
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

from __future__ import annotations

from typing import Tuple, Type

from crossbench.probes.android_logcat import AndroidLogcatProbe
from crossbench.probes.debugger import DebuggerProbe
from crossbench.probes.dtrace import DTraceProbe
from crossbench.probes.helper import INTERNAL_NAME_PREFIX
from crossbench.probes.internal import (DurationsProbe, ErrorsProbe,
                                        InternalProbe, LogProbe,
                                        ResultsSummaryProbe, SystemDetailsProbe)
from crossbench.probes.js import JSProbe
from crossbench.probes.json import JsonResultProbe
from crossbench.probes.perfetto import PerfettoProbe
from crossbench.probes.performance_entries import PerformanceEntriesProbe
from crossbench.probes.polling import ShellPollingProbe
from crossbench.probes.power_sampler import PowerSamplerProbe
from crossbench.probes.powermetrics import PowerMetricsProbe
from crossbench.probes.probe import Probe
from crossbench.probes.profiling.browser_profiling import BrowserProfilingProbe
from crossbench.probes.profiling.system_profiling import ProfilingProbe
from crossbench.probes.screenshot import ScreenshotProbe
from crossbench.probes.shell import ShellProbe
from crossbench.probes.system_stats import SystemStatsProbe
from crossbench.probes.trace_processor.trace_processor import \
    TraceProcessorProbe
from crossbench.probes.tracing import TracingProbe
from crossbench.probes.v8.builtins_pgo import V8BuiltinsPGOProbe
from crossbench.probes.v8.log import V8LogProbe
from crossbench.probes.v8.rcs import V8RCSProbe
from crossbench.probes.v8.turbolizer import V8TurbolizerProbe
from crossbench.probes.video import VideoProbe
from crossbench.probes.web_page_replay.recorder import WebPageReplayProbe

ABSTRACT_PROBES: Tuple[Type[Probe], ...] = (Probe, JsonResultProbe)

# Probes that are not user-configurable
# Order matters, not alpha-sorted:
# Internal probes depend on each other, for instance the ResultsSummaryProbe
# reads the values of the other internal probes and thus needs to be the first
# to be initialized and the last to be teared down to write out a summary
# result of all the other probes.
INTERNAL_PROBES: Tuple[Type[InternalProbe], ...] = (
    ResultsSummaryProbe,
    DurationsProbe,
    ErrorsProbe,
    LogProbe,
    SystemDetailsProbe,
)
# ResultsSummaryProbe should always be processed last, and thus must be the
# first probe to be added to any browser.
assert INTERNAL_PROBES[0] == ResultsSummaryProbe
assert INTERNAL_PROBES[1] == DurationsProbe

# Probes that can be used on arbitrary stories and may be user configurable.
GENERAL_PURPOSE_PROBES: Tuple[Type[Probe], ...] = (
    AndroidLogcatProbe,
    BrowserProfilingProbe,
    DTraceProbe,
    DebuggerProbe,
    JSProbe,
    PerfettoProbe,
    PerformanceEntriesProbe,
    PowerMetricsProbe,
    PowerSamplerProbe,
    ProfilingProbe,
    ScreenshotProbe,
    ShellPollingProbe,
    ShellProbe,
    SystemStatsProbe,
    TraceProcessorProbe,
    TracingProbe,
    V8BuiltinsPGOProbe,
    V8LogProbe,
    V8RCSProbe,
    V8TurbolizerProbe,
    VideoProbe,
    WebPageReplayProbe,
)

for probe_cls in GENERAL_PURPOSE_PROBES:
  assert probe_cls.IS_GENERAL_PURPOSE, (
      f"Probe {probe_cls} should be marked for GENERAL_PURPOSE")
  assert probe_cls.NAME
  assert not probe_cls.NAME.startswith(INTERNAL_NAME_PREFIX), (
      f"General purpose {probe_cls}.NAME cannot start with 'cb.'")

for probe_cls in INTERNAL_PROBES:
  assert not probe_cls.IS_GENERAL_PURPOSE, (
      f"Internal Probe {probe_cls} should not marked for GENERAL_PURPOSE")
  assert probe_cls.NAME
  assert probe_cls.NAME.startswith(INTERNAL_NAME_PREFIX), (
      f"Internal {probe_cls}.NAME must start with 'cb.'")
