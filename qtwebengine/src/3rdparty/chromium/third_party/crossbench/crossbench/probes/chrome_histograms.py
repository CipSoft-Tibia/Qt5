# Copyright 2024 The Chromium Authors
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

from __future__ import annotations

import abc
import argparse
import dataclasses
import functools
import logging
import re
from typing import TYPE_CHECKING, Any, Dict, List, Optional, Sequence, Type

from crossbench.browsers.attributes import BrowserAttributes
from crossbench.parse import ObjectParser
from crossbench.probes.json import JsonResultProbe, JsonResultProbeContext
from crossbench.probes.probe import ProbeConfigParser
from crossbench.probes.result_location import ResultLocation

if TYPE_CHECKING:
  from crossbench.browsers.browser import Browser
  from crossbench.env import HostEnvironment
  from crossbench.runner.actions import Actions
  from crossbench.runner.run import Run
  from crossbench.types import Json


class ChromeHistogramMetric(abc.ABC):
  """
  Stores enough information to log a single metric from a diff between two UMA
  histograms.
  """

  def __init__(self, name: str, histogram_name: str) -> None:
    super().__init__()
    self._name = name
    self._histogram_name = histogram_name

  @property
  def name(self) -> str:
    return self._name

  @property
  def histogram_name(self) -> str:
    return self._histogram_name

  @abc.abstractmethod
  def compute(self, delta: ChromeHistogramSample,
              baseline: ChromeHistogramSample) -> float:
    pass


class ChromeHistogramCountMetric(ChromeHistogramMetric):

  def __init__(self, histogram_name: str):
    super().__init__(f"{histogram_name}_count", histogram_name)

  def compute(self, delta: ChromeHistogramSample,
              baseline: ChromeHistogramSample) -> float:
    return delta.diff_count(baseline)


class ChromeHistogramMeanMetric(ChromeHistogramMetric):

  def __init__(self, histogram_name: str):
    super().__init__(f"{histogram_name}_mean", histogram_name)

  def compute(self, delta: ChromeHistogramSample,
              baseline: ChromeHistogramSample) -> float:
    return delta.diff_mean(baseline)


class ChromeHistogramPercentileMetric(ChromeHistogramMetric):

  def __init__(self, histogram_name: str, percentile: int):
    super().__init__(f"{histogram_name}_p{percentile}", histogram_name)
    self._percentile = percentile

  def compute(self, delta: ChromeHistogramSample,
              baseline: ChromeHistogramSample) -> float:
    return delta.diff_percentile(baseline, self._percentile)


PERCENTILE_METRIC_RE = re.compile(r"^p(\d+)$")


def parse_histogram_metrics(value: Any,
                            name: str = "value"
                           ) -> Sequence[ChromeHistogramMetric]:
  result: List[ChromeHistogramMetric] = []
  d = ObjectParser.dict(value, name)
  for k, v in d.items():
    histogram_name = ObjectParser.any_str(k, f"{name} name")
    metrics = ObjectParser.non_empty_sequence(
        v, f"{name} {histogram_name} metrics")
    for x in metrics:
      metric = ObjectParser.any_str(x)
      if metric == "count":
        result.append(ChromeHistogramCountMetric(histogram_name))
      elif metric == "mean":
        result.append(ChromeHistogramMeanMetric(histogram_name))
      else:
        m = re.match(PERCENTILE_METRIC_RE, metric)
        if not m:
          raise argparse.ArgumentTypeError(
              f"{name} {histogram_name} {metric} is not a valid metric")
        percentile = int(m[1])
        if percentile < 0 or percentile > 100:
          raise argparse.ArgumentTypeError(
              f"{name} {histogram_name} {metric} is not a valid percentile")
        result.append(
            ChromeHistogramPercentileMetric(histogram_name, percentile))
  return result


class ChromeHistogramsProbe(JsonResultProbe):
  """
  Probe that collects UMA histogram metrics from Chrome.
  """
  NAME = "chrome_histograms"
  RESULT_LOCATION = ResultLocation.LOCAL

  @classmethod
  def config_parser(cls) -> ProbeConfigParser:
    parser = super().config_parser()
    parser.add_argument(
        "metrics",
        required=True,
        type=parse_histogram_metrics,
        help=("Required dictionary of Chrome UMA histogram metric names. "
              "Histograms are recorded before and after a test and any "
              "differences logged."
              "See tools/metrics/histograms/metadata/storage/histograms.xml"
              "or chrome://histograms for a list of available histograms."))
    return parser

  def __init__(self, metrics: Sequence[ChromeHistogramMetric]) -> None:
    super().__init__()
    self._metrics = metrics

  @property
  def metrics(self) -> Sequence[ChromeHistogramMetric]:
    return self._metrics

  def validate_browser(self, env: HostEnvironment, browser: Browser) -> None:
    super().validate_browser(env, browser)
    self.expect_browser(browser, BrowserAttributes.CHROMIUM_BASED)

  def get_context_cls(self) -> Type[ChromeHistogramsProbeContext]:
    return ChromeHistogramsProbeContext


@dataclasses.dataclass
class ChromeHistogramBucket:
  min: int
  max: Optional[int]
  count: int


ChromeHistogramBuckets = List[ChromeHistogramBucket]


class ChromeHistogramSample:
  """
  Stores the contents of one UMA histogram and provides helpers to generate
  metrics based on the difference between two samples.
  """

  # Generated by https://source.chromium.org/chromium/chromium/src/+/main:base/metrics/sample_vector.cc;l=520;drc=de573334f8fa97f9a7c99577611302736d2490b6
  # Example histogram body lines (with whitespace shortened):
  # "1326111536  -------------------O                              (19 = 63.3%)"
  # "114   ---O                                              (3 = 3.1%) {92.7%}"
  # "12  ... "
  # "1000..."
  _BUCKET_RE = re.compile(
      r"^(-?\d+) *(?:(?:-*O "  # Bucket min and ASCII bar
      r"+\((\d+) = \d+\.\d%\)(?: \{\d+\.\d%\}"  # Count and optional sum %
      r")?)|(?:\.\.\. ))$"  # Or a "..." line
  )

  # Generated by https://source.chromium.org/chromium/chromium/src/+/main:base/metrics/sample_vector.cc;l=538;drc=de573334f8fa97f9a7c99577611302736d2490b6
  # Example histogram header lines:
  # "Histogram: UKM.InitSequence recorded 1 samples, mean = 1.0 (flags = 0x41)"
  # "Histogram: WebUI.CreatedForUrl recorded 30 samples (flags = 0x41)"
  _HEADER_RE = re.compile(r"^Histogram: +.* recorded (\d+) samples"
                          r"(?:, mean = (-?\d+\.\d+))?"
                          r" \(flags = (0x[0-9A-Fa-f]+)\)$")

  @classmethod
  def from_json(cls, histogram_dict: Dict) -> ChromeHistogramSample:
    name = ObjectParser.any_str(histogram_dict["name"], "histogram name")
    header = ObjectParser.any_str(histogram_dict["header"], "histogram header")
    body = ObjectParser.any_str(histogram_dict["body"], "histogram body")

    m = re.match(cls._HEADER_RE, header)
    if not m:
      raise argparse.ArgumentTypeError(
          f"{name} histogram header has invalid data: {header}")
    count = int(m.group(1))
    mean = float(m.group(2)) if m.group(2) is not None else None
    flags = int(m.group(3), 16)

    bucket_counts: Dict[int, int] = {}
    bucket_maxes: Dict[int, int] = {}
    prev_min: Optional[int] = None
    for i, line in enumerate(body.splitlines(), start=1):
      m = re.match(cls._BUCKET_RE, line)
      if not m:
        raise argparse.ArgumentTypeError(
            f"{name} histogram body line {i} has invalid data: {line}")

      bucket_min = int(m.group(1))

      # Previous bucket's max is this bucket's min.
      if prev_min is not None:
        bucket_maxes[prev_min] = bucket_min
      prev_min = bucket_min

      if bucket_count_str := m.group(2):
        bucket_count = int(bucket_count_str)
        if bucket_count > 0:
          bucket_counts[bucket_min] = bucket_count
    return ChromeHistogramSample(name, count, mean, flags, bucket_counts,
                                 bucket_maxes)

  def __init__(self,
               name: str,
               count: int = 0,
               mean: Optional[float] = 0,
               flags: int = 0,
               bucket_counts: Optional[Dict[int, int]] = None,
               bucket_maxes: Optional[Dict[int, int]] = None):
    self._name = name
    self._count = count
    self._mean = mean
    self._flags = flags
    self._bucket_counts = bucket_counts or {}
    self._bucket_maxes = bucket_maxes or {}
    bucket_sum = sum(self._bucket_counts.values())
    if count != bucket_sum:
      raise ValueError(f"Histogram {name} has {count} total samples, "
                       f"but buckets add to {bucket_sum}")

  @property
  def mean(self) -> Optional[float]:
    return self._mean

  @property
  def count(self) -> int:
    return self._count

  def bucket_max(self, bucket_min: int) -> Optional[int]:
    return self._bucket_maxes.get(bucket_min)

  def bucket_count(self, bucket_min: int) -> int:
    return self._bucket_counts.get(bucket_min, 0)

  def diff_buckets(self,
                   baseline: ChromeHistogramSample) -> ChromeHistogramBuckets:
    buckets: ChromeHistogramBuckets = []
    for bucket_min, bucket_count in self._bucket_counts.items():
      bucket_count = bucket_count - baseline.bucket_count(bucket_min)
      bucket_max: Optional[int] = self._bucket_maxes.get(bucket_min)
      buckets.append(
          ChromeHistogramBucket(bucket_min, bucket_max, bucket_count))
    return buckets

  def diff_percentile(self, baseline: ChromeHistogramSample,
                      percentile: int) -> float:
    if percentile < 0 or percentile > 100:
      raise ValueError(f"{percentile} is not a valid percentile")
    buckets = self.diff_buckets(baseline)
    count = functools.reduce(lambda s, b: b.count + s, buckets, 0)
    if count == 0:
      raise ValueError(
          f"{self._name} can not compute percentile without any samples")
    target = count * percentile / 100
    for bucket in buckets:
      if target <= bucket.count:
        if bucket.max is None:
          return bucket.min
        # Assume all samples are evenly distributed within the bucket.
        # NB: 0 <= target <= bucket_count
        t = target / (bucket.count + 1)
        return bucket.min * (1 - t) + bucket.max * t
      target -= bucket.count
    raise ValueError("overflowed histogram buckets looking for percentile")

  def diff_mean(self, baseline: ChromeHistogramSample) -> float:
    count = self._count - baseline.count
    if count <= 0:
      raise ValueError(f"{self._name} can not compute mean without any samples")
    if self._mean is None or baseline.mean is None:
      raise ValueError(
          f"{self._name} has no mean reported, is it an enum histogram?")

    return (self._mean * self._count - baseline.mean * baseline.count) / count

  def diff_count(self, baseline: ChromeHistogramSample) -> int:
    return self._count - baseline.count

  @property
  def name(self) -> str:
    return self._name


class ChromeHistogramsProbeContext(JsonResultProbeContext[ChromeHistogramsProbe]
                                  ):

  # JS code that overrides the chrome.send response handler and requests
  # histograms.
  HISTOGRAM_SEND = """
function webUIResponse(id, isSuccess, response) {
  if (id === "crossbench_histograms_1") {
    window.crossbench_histograms = response;
  }
}
window.cr.webUIResponse = webUIResponse;
chrome.send("requestHistograms", ["crossbench_histograms_1", "", true]);
"""

  # JS code that checks if there is a histogram response.
  HISTOGRAM_WAIT = "return !!window.crossbench_histograms"

  # JS code that returns the histograms response.
  HISTOGRAM_DATA = "return window.crossbench_histograms"

  def __init__(self, probe: ChromeHistogramsProbe, run: Run) -> None:
    super().__init__(probe, run)
    self._baseline: Optional[Dict[str, ChromeHistogramSample]] = None
    self._delta: Optional[Dict[str, ChromeHistogramSample]] = None

  def dump_histograms(self, name: str) -> Dict[str, ChromeHistogramSample]:
    with self.run.actions(
        f"Probe({self.probe.name}) dump histograms {name}") as actions:
      actions.show_url("chrome://histograms")
      actions.js(self.HISTOGRAM_SEND)
      actions.wait_js_condition(self.HISTOGRAM_WAIT, 0.1, 10.0)
      data = actions.js(self.HISTOGRAM_DATA)
      histogram_list = ObjectParser.sequence(data)
      histograms: Dict[str, ChromeHistogramSample] = {}
      for histogram_dict in histogram_list:
        histogram = ChromeHistogramSample.from_json(
            ObjectParser.dict(histogram_dict))
        histograms[histogram.name] = histogram
      return histograms

  def start(self) -> None:
    self._baseline = self.dump_histograms("start")
    super().start()

  def stop(self) -> None:
    self._delta = self.dump_histograms("stop")
    super().stop()

  def to_json(self, actions: Actions) -> Json:
    del actions
    assert self._baseline, "Did not extract start histograms"
    assert self._delta, "Did not extract end histograms"
    json = {}
    for metric in self.probe.metrics:
      baseline = self._baseline.get(
          metric.histogram_name, ChromeHistogramSample(metric.histogram_name))
      delta = self._delta.get(metric.histogram_name,
                              ChromeHistogramSample(metric.histogram_name))
      try:
        json[metric.name] = metric.compute(delta, baseline)
      except Exception as e:  # pylint: disable=broad-exception-caught
        logging.warning("Failed to log metric %s: %s", metric.name, e)
    return json
