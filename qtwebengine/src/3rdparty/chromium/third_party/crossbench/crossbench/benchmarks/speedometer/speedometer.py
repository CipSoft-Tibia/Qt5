# Copyright 2023 The Chromium Authors
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

from __future__ import annotations

import abc
import datetime as dt
import json
import logging
from typing import (TYPE_CHECKING, Any, Dict, List, Optional, Sequence, Tuple,
                    Type)

import crossbench.probes.helper as probes_helper
from crossbench import cli_helper, helper
from crossbench.benchmarks.base import (BenchmarkProbeMixin, PressBenchmark,
                                        PressBenchmarkStoryFilter)
from crossbench.probes import metric as cb_metric
from crossbench.probes.json import JsonResultProbe
from crossbench.probes.results import ProbeResult, ProbeResultDict
from crossbench.stories.press_benchmark import PressBenchmarkStory

if TYPE_CHECKING:
  import argparse

  from crossbench.path import LocalPath
  from crossbench.runner.actions import Actions
  from crossbench.runner.groups import BrowsersRunGroup, StoriesRunGroup
  from crossbench.runner.run import Run
  from crossbench.types import Json


def _probe_remove_tests_segments(path: Tuple[str, ...]) -> str:
  return "/".join(segment for segment in path if segment != "tests")


class SpeedometerProbe(
    BenchmarkProbeMixin, JsonResultProbe, metaclass=abc.ABCMeta):
  """
  Speedometer-specific probe (compatible with v2.X and v3.X).
  Extracts all speedometer times and scores.
  """
  JS: str = "return window.suiteValues;"
  SORT_KEYS: bool = False

  def to_json(self, actions: Actions) -> Json:
    return actions.js(self.JS)

  def flatten_json_data(self, json_data: Any) -> Json:
    # json_data may contain multiple iterations, merge those first
    assert isinstance(json_data, list), f"Expected list got {type(json_data)}"
    merged = cb_metric.MetricsMerger(
        json_data, key_fn=_probe_remove_tests_segments).to_json(
            value_fn=lambda values: values.geomean, sort=self.SORT_KEYS)
    return probes_helper.Flatten(merged, sort=self.SORT_KEYS).data

  def merge_stories(self, group: StoriesRunGroup) -> ProbeResult:
    merged = cb_metric.MetricsMerger.merge_json_list(
        repetitions_group.results[self].json
        for repetitions_group in group.repetitions_groups)
    return self.write_group_result(group, merged, write_csv=True)

  def merge_browsers(self, group: BrowsersRunGroup) -> ProbeResult:
    return self.merge_browsers_json_list(group).merge(
        self.merge_browsers_csv_list(group))

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
    logging.critical("Speedometer results:")
    if not single_result:
      logging.critical("  %s", result_dict[self].csv)
    logging.info("- " * 40)

    with results_json.open(encoding="utf-8") as f:
      data = json.load(f)
      if single_result:
        score = data.get("score") or data["Score"]
        logging.critical("Score %s", score)
      else:
        self._log_result_metrics(data)

  def _extract_result_metrics_table(self, metrics: Dict[str, Any],
                                    table: Dict[str, List[str]]) -> None:
    for metric_key, metric in metrics.items():
      if not self._valid_metric_key(metric_key):
        continue
      table[metric_key].append(
          cb_metric.format_metric(metric["average"], metric["stddev"]))

  @abc.abstractmethod
  def _valid_metric_key(self, metric_key: str) -> bool:
    pass



class SpeedometerStory(PressBenchmarkStory, metaclass=abc.ABCMeta):
  URL_LOCAL: str = "http://localhost:8000/"
  DEFAULT_ITERATIONS: int = 10

  def __init__(self,
               substories: Sequence[str] = (),
               iterations: Optional[int] = None,
               url: Optional[str] = None):
    self._iterations: int = iterations or self.DEFAULT_ITERATIONS
    assert self.iterations >= 1, f"Invalid iterations count: '{iterations}'."
    super().__init__(url=url, substories=substories)

  @property
  def iterations(self) -> int:
    return self._iterations

  @property
  def substory_duration(self) -> dt.timedelta:
    return self.iterations * dt.timedelta(seconds=0.4)

  @property
  def slow_duration(self) -> dt.timedelta:
    """Max duration that covers run-times on slow machines and/or
    debug-mode browsers.
    Making this number too large might cause needless wait times on broken
    browsers/benchmarks.
    """
    return dt.timedelta(seconds=60 * 20) + self.duration * 10

  @property
  def url_params(self) -> Dict[str, str]:
    if self.iterations == self.DEFAULT_ITERATIONS:
      return {}
    return {"iterationCount": str(self.iterations)}

  def setup(self, run: Run) -> None:
    updated_url = helper.update_url_query(self.url, self.url_params)
    if updated_url != self.url:
      logging.info("CUSTOM URL: %s", updated_url)

    with run.actions("Setup") as actions:
      actions.show_url(updated_url)
      actions.wait_js_condition("return window.Suites !== undefined;", 0.5, 10)
      self._setup_substories(actions)
      self._setup_benchmark_client(actions)
      actions.wait(0.5)

  def _setup_substories(self, actions: Actions) -> None:
    if self._substories == self.SUBSTORIES:
      return
    actions.js(
        """
        let substories = arguments[0];
        Suites.forEach((suite) => {
          suite.disabled = substories.indexOf(suite.name) === -1;
        });""",
        arguments=[self._substories])

  def _setup_benchmark_client(self, actions: Actions) -> None:
    actions.js("""
      window.testDone = false;
      window.suiteValues = [];
      const client = window.benchmarkClient;
      const clientCopy = {
        didRunSuites: client.didRunSuites,
        didFinishLastIteration: client.didFinishLastIteration,
      };
      client.didRunSuites = function(measuredValues, ...arguments) {
          clientCopy.didRunSuites.call(this, measuredValues, ...arguments);
          window.suiteValues.push(measuredValues);
      };
      client.didFinishLastIteration = function(...arguments) {
          clientCopy.didFinishLastIteration.call(this, ...arguments);
          window.testDone = true;
      };""")

  def run(self, run: Run) -> None:
    with run.actions("Running") as actions:
      actions.js("""
          if (window.startTest) {
            window.startTest();
          } else {
            // Interactive Runner fallback / old 3.0 fallback.
            let startButton = document.getElementById("runSuites") ||
                document.querySelector("start-tests-button") ||
                document.querySelector(".buttons button");
            startButton.click();
          }
          """)
      actions.wait(self.fast_duration)
    with run.actions("Waiting for completion") as actions:
      actions.wait_js_condition("return window.testDone",
                                self.substory_duration, self.slow_duration)


ProbeClsTupleT = Tuple[Type[SpeedometerProbe], ...]


class SpeedometerBenchmarkStoryFilter(PressBenchmarkStoryFilter):
  __doc__ = PressBenchmarkStoryFilter.__doc__

  @classmethod
  def add_cli_parser(
      cls, parser: argparse.ArgumentParser) -> argparse.ArgumentParser:
    parser = super().add_cli_parser(parser)
    parser.add_argument(
        "--iterations",
        "--iteration-count",
        default=SpeedometerStory.DEFAULT_ITERATIONS,
        type=cli_helper.parse_positive_int,
        help="Number of iterations each Speedometer subtest is run "
        "within the same session. \n"
        "Note: --repeat restarts the whole benchmark, --iterations runs the"
        "same test tests n-times within the same session without the setup "
        "overhead of starting up a whole new browser.")
    return parser

  @classmethod
  def kwargs_from_cli(cls, args: argparse.Namespace) -> Dict[str, Any]:
    kwargs = super().kwargs_from_cli(args)
    kwargs["iterations"] = args.iterations
    return kwargs

  def __init__(self,
               story_cls: Type[SpeedometerStory],
               patterns: Sequence[str],
               separate: bool = False,
               url: Optional[str] = None,
               iterations: Optional[int] = None):
    self.iterations = iterations
    assert issubclass(story_cls, SpeedometerStory)
    super().__init__(story_cls, patterns, separate, url)

  def create_stories_from_names(self, names: List[str],
                                separate: bool) -> Sequence[SpeedometerStory]:
    return self.story_cls.from_names(
        names, separate=separate, url=self.url, iterations=self.iterations)


class SpeedometerBenchmark(PressBenchmark, metaclass=abc.ABCMeta):

  DEFAULT_STORY_CLS = SpeedometerStory
  STORY_FILTER_CLS = SpeedometerBenchmarkStoryFilter

  @classmethod
  def short_base_name(cls) -> str:
    return "sp"

  @classmethod
  def base_name(cls) -> str:
    return "speedometer"
