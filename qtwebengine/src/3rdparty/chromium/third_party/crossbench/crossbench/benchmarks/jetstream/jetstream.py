# Copyright 2024 The Chromium Authors
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

from __future__ import annotations

import abc
import json
import logging
from collections import defaultdict
from typing import (TYPE_CHECKING, Any, Dict, Final, List, Optional, Sequence,
                    Tuple, Type, cast)

from crossbench.benchmarks.base import PressBenchmark
from crossbench.benchmarks.benchmark_probe import BenchmarkProbeMixin
from crossbench.parse import ObjectParser
from crossbench.probes.json import JsonResultProbe, JsonResultProbeContext
from crossbench.probes.metric import (CSVFormatter, Metric, MetricsMerger,
                                      geomean)
from crossbench.probes.results import ProbeResult, ProbeResultDict

if TYPE_CHECKING:
  import argparse

  from crossbench.cli.parser import CrossBenchArgumentParser
  from crossbench.path import LocalPath
  from crossbench.runner.actions import Actions
  from crossbench.runner.groups.browsers import BrowsersRunGroup
  from crossbench.runner.groups.stories import StoriesRunGroup
  from crossbench.runner.run import Run
  from crossbench.stories.story import Story
  from crossbench.types import Json


class JetStreamProbe(
    BenchmarkProbeMixin, JsonResultProbe, metaclass=abc.ABCMeta):
  """
  JetStream-specific Probe.
  Extracts all JetStream times and scores.
  """

  TOTAL_METRIC_KEY: Final[str] = "Total/score"

  @property
  def jetstream(self) -> JetStreamBenchmark:
    return cast(JetStreamBenchmark, self.benchmark)

  @abc.abstractmethod
  def get_context_cls(self) -> Type[JetStreamProbeContext]:
    pass

  def log_run_result(self, run: Run) -> None:
    self._log_result(run.results, single_result=True)

  def log_browsers_result(self, group: BrowsersRunGroup) -> None:
    self._log_result(group.results, single_result=False)

  def _log_result(self, result_dict: ProbeResultDict,
                  single_result: bool) -> None:
    if self not in result_dict:
      return
    results_json: LocalPath = result_dict[self].json
    logging.info("-" * 80)
    logging.critical("JetStream results:")
    if not single_result:
      logging.critical("  %s", result_dict[self].csv)
    logging.info("- " * 40)

    with results_json.open(encoding="utf-8") as f:
      data = json.load(f)
      if single_result:
        logging.critical("Score %s", data["Total"]["score"])
      else:
        self._log_result_metrics(data)

  def _extract_result_metrics_table(self, metrics: Dict[str, Any],
                                    table: Dict[str, List[str]]) -> None:
    for metric_key, metric_value in metrics.items():
      if not self._is_valid_metric_key(metric_key):
        continue
      table[metric_key].append(
          Metric.format(metric_value["average"], metric_value["stddev"]))
      # Separate runs don't produce a score
    if self.TOTAL_METRIC_KEY in metrics:
      metric_value = metrics[self.TOTAL_METRIC_KEY]
      table["Score"].append(
          Metric.format(metric_value["average"], metric_value["stddev"]))

  def merge_stories(self, group: StoriesRunGroup) -> ProbeResult:
    merged = MetricsMerger.merge_json_list(
        story_group.results[self].json
        for story_group in group.repetitions_groups)
    # We discard the score when merging separate line item runs, recompute it!
    if self.TOTAL_METRIC_KEY not in merged.data:
      merged.data[self.TOTAL_METRIC_KEY] = self._compute_total_score(merged)
    return self.write_group_result(group, merged, JetStreamCSVFormatter)

  def _compute_total_score(self, merged: MetricsMerger) -> Metric:
    line_item_scores: List[List[float]] = []
    for key, metric in merged.data.items():
      if self._is_valid_metric_key(key):
        line_item_scores.append(metric.values)
    total_score = Metric()
    for iteration_line_items_score_values in zip(*line_item_scores):
      iteration_score = Metric(iteration_line_items_score_values).geomean
      total_score.append(iteration_score)
    return total_score

  def merge_browsers(self, group: BrowsersRunGroup) -> ProbeResult:
    return self.merge_browsers_json_list(group).merge(
        self.merge_browsers_csv_list(group))

  def _is_valid_metric_key(self, metric_key: str) -> bool:
    parts = metric_key.split("/")
    if len(parts) != 2:
      return False
    if self.jetstream.detailed_metrics:
      return True
    return parts[0] != "Total" and parts[1] == "score"


class JetStreamProbeContext(JsonResultProbeContext):
  FLATTEN: bool = False
  JS: str = """
  let results = Object.create(null);
  let benchmarks = []
  for (let benchmark of JetStream.benchmarks) {
    const data = { score: benchmark.score };
    if ("worst4" in benchmark) {
      data.firstIteration = benchmark.firstIteration;
      data.average = benchmark.average;
      data.worst4 = benchmark.worst4;
    } else if ("runTime" in benchmark) {
      data.runTime = benchmark.runTime;
      data.startupTime = benchmark.startupTime;
    } else if ("mainRun" in benchmark) {
      data.mainRun = benchmark.mainRun;
      data.stdlib = benchmark.stdlib;
    }
    results[benchmark.plan.name] = data;
    benchmarks.push(benchmark);
  };
  return JSON.stringify(results);
"""

  def to_json(self, actions: Actions) -> Dict[str, float]:
    # Use serialized json as transport format to preserve object key order.
    json_payload = actions.js(self.JS)
    json_data = json.loads(json_payload)
    ObjectParser.non_empty_dict(json_data, f"{self.probe.name} metrics")
    return json_data

  def process_json_data(self, json_data: Json) -> Json:
    assert isinstance(json_data, dict)
    assert "Total" not in json_data, (
        "JSON result data already contains a ['Total'] entry.")
    json_data["Total"] = self._compute_total_metrics(json_data)
    return json_data

  def _compute_total_metrics(self, json_data: Dict[str,
                                                   Any]) -> Dict[str, float]:
    # Manually add all total scores
    accumulated_metrics = defaultdict(list)
    for _, metrics in json_data.items():
      for metric, value in metrics.items():
        accumulated_metrics[metric].append(value)
    total: Dict[str, float] = {}
    for metric, values in accumulated_metrics.items():
      total[metric] = geomean(values)
    return total


class JetStreamCSVFormatter(CSVFormatter):
  TOTAL_METRIC_KEY: Final[str] = JetStreamProbe.TOTAL_METRIC_KEY

  def format_items(self, data: Dict[str, Json],
                   sort: bool) -> Sequence[Tuple[str, Json]]:
    items = list(data.items())
    if sort:
      items.sort()
    # Copy all /score items to the top:
    score_items = []
    for key, value in items:
      if key != self.TOTAL_METRIC_KEY and key.endswith("/score"):
        score_items.append((key, value))
    total_item = [(self.TOTAL_METRIC_KEY, data[self.TOTAL_METRIC_KEY])]
    return total_item + score_items + items


class JetStreamBenchmark(PressBenchmark, metaclass=abc.ABCMeta):

  @classmethod
  def short_base_name(cls) -> str:
    return "js"

  @classmethod
  def base_name(cls) -> str:
    return "jetstream"

  @classmethod
  def add_cli_parser(
      cls, subparsers: argparse.ArgumentParser, aliases: Sequence[str] = ()
  ) -> CrossBenchArgumentParser:
    parser = super().add_cli_parser(subparsers, aliases)
    parser.add_argument(
        "--detailed-metrics",
        "--details",
        default=False,
        action="store_true",
        help="Report more detailed internal metrics.")
    return parser

  @classmethod
  def kwargs_from_cli(cls, args: argparse.Namespace) -> Dict[str, Any]:
    kwargs = super().kwargs_from_cli(args)
    kwargs["detailed_metrics"] = args.detailed_metrics
    return kwargs

  def __init__(self,
               stories: Sequence[Story],
               custom_url: Optional[str] = None,
               detailed_metrics: bool = False):
    self._detailed_metrics = detailed_metrics
    super().__init__(stories, custom_url)

  @property
  def detailed_metrics(self) -> bool:
    return self._detailed_metrics
