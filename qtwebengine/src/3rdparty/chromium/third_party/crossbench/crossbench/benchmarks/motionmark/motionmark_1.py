# Copyright 2024 The Chromium Authors
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

from __future__ import annotations

import abc
import datetime as dt
import itertools
import json
import logging
from typing import TYPE_CHECKING, Any, Dict, List, Optional, Tuple

import crossbench.probes.helper as probes_helper
from crossbench.benchmarks.base import BenchmarkProbeMixin
from crossbench.benchmarks.motionmark.base import MotionMarkBenchmark
from crossbench.helper import update_url_query
from crossbench.probes import metric as cb_metric
from crossbench.probes.json import JsonResultProbe
from crossbench.probes.results import ProbeResult, ProbeResultDict
from crossbench.stories.press_benchmark import PressBenchmarkStory

if TYPE_CHECKING:
  from crossbench.path import LocalPath
  from crossbench.runner.actions import Actions
  from crossbench.runner.groups import BrowsersRunGroup, StoriesRunGroup
  from crossbench.runner.run import Run
  from crossbench.types import Json


def _clean_up_path_segments(path: Tuple[str, ...]) -> Optional[str]:
  name = path[-1]
  if name.startswith("segment") or name == "data":
    return None
  if path[:2] == ("testsResults", "MotionMark"):
    path = path[2:]
  return "/".join(path)


class MotionMark1Probe(BenchmarkProbeMixin, JsonResultProbe, abc.ABC):
  """
  MotionMark-specific Probe.
  Extracts all MotionMark times and scores.
  """
  JS = """
    return window.benchmarkRunnerClient.results.results;
  """

  def to_json(self, actions: Actions) -> Json:
    return actions.js(self.JS)

  def flatten_json_data(self, json_data: List) -> Json:
    assert isinstance(json_data, list) and len(json_data) == 1, (
        "Motion12MarkProbe requires a results list.")
    return probes_helper.Flatten(
        json_data[0], key_fn=_clean_up_path_segments).data

  def merge_stories(self, group: StoriesRunGroup) -> ProbeResult:
    merged = cb_metric.MetricsMerger.merge_json_list(
        story_group.results[self].json
        for story_group in group.repetitions_groups)
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
    logging.critical("Motionmark results:")
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
    # Separate runs don't produce a score
    if total_metric := metrics.get("score") or metrics.get("Score"):
      table["Score"].append(
          cb_metric.format_metric(total_metric["average"],
                                  total_metric["stddev"]))

  def _valid_metric_key(self, metric_key: str) -> bool:
    parts = metric_key.split("/")
    return len(parts) == 2 or parts[-1] == "score"


class MotionMark1Story(PressBenchmarkStory):
  URL_LOCAL: str = "http://localhost:8000/"
  ALL_STORIES = {
      "MotionMark": (
          "Multiply",
          "Canvas Arcs",
          "Leaves",
          "Paths",
          "Canvas Lines",
          "Images",
          "Design",
          "Suits",
      ),
      "HTML suite": (
          "CSS bouncing circles",
          "CSS bouncing clipped rects",
          "CSS bouncing gradient circles",
          "CSS bouncing blend circles",
          "CSS bouncing filter circles",
          # "CSS bouncing SVG images",
          "CSS bouncing tagged images",
          "Focus 2.0",
          "DOM particles, SVG masks",
          # "Composited Transforms",
      ),
      "Canvas suite": (
          "canvas bouncing clipped rects",
          "canvas bouncing gradient circles",
          # "canvas bouncing SVG images",
          # "canvas bouncing PNG images",
          "Stroke shapes",
          "Fill shapes",
          "Canvas put/get image data",
      ),
      "SVG suite": (
          "SVG bouncing circles",
          "SVG bouncing clipped rects",
          "SVG bouncing gradient circles",
          # "SVG bouncing SVG images",
          # "SVG bouncing PNG images",
      ),
      "Leaves suite": (
          "Translate-only Leaves",
          "Translate + Scale Leaves",
          "Translate + Opacity Leaves",
      ),
      "Multiply suite": (
          "Multiply: CSS opacity only",
          "Multiply: CSS display only",
          "Multiply: CSS visibility only",
      ),
      "Text suite": (
          "Design: Latin only (12 items)",
          "Design: CJK only (12 items)",
          "Design: RTL and complex scripts only (12 items)",
          "Design: Latin only (6 items)",
          "Design: CJK only (6 items)",
          "Design: RTL and complex scripts only (6 items)",
      ),
      "Suits suite": (
          "Suits: clip only",
          "Suits: shape only",
          "Suits: clip, shape, rotation",
          "Suits: clip, shape, gradient",
          "Suits: static",
      ),
      "3D Graphics": (
          "Triangles (WebGL)",
          # "Triangles (WebGPU)",
      ),
      "Basic canvas path suite": (
          "Canvas line segments, butt caps",
          "Canvas line segments, round caps",
          "Canvas line segments, square caps",
          "Canvas line path, bevel join",
          "Canvas line path, round join",
          "Canvas line path, miter join",
          "Canvas line path with dash pattern",
          "Canvas quadratic segments",
          "Canvas quadratic path",
          "Canvas bezier segments",
          "Canvas bezier path",
          "Canvas arcTo segments",
          "Canvas arc segments",
          "Canvas rects",
          "Canvas ellipses",
          "Canvas line path, fill",
          "Canvas quadratic path, fill",
          "Canvas bezier path, fill",
          "Canvas arcTo segments, fill",
          "Canvas arc segments, fill",
          "Canvas rects, fill",
          "Canvas ellipses, fill",
      )
  }
  SUBSTORIES = tuple(itertools.chain.from_iterable(ALL_STORIES.values()))
  DEVELOPER_READY_JS: str = (
      "return document.querySelector('tree > li') !== undefined;")
  # The default page is ready immediately.
  READY_JS = "return true;"

  @classmethod
  def default_story_names(cls) -> Tuple[str, ...]:
    return cls.ALL_STORIES["MotionMark"]

  @property
  def substory_duration(self) -> dt.timedelta:
    return dt.timedelta(seconds=35)

  @property
  def url_params(self) -> Dict[str, str]:
    return {}

  def prepare_test_url(self) -> str:
    if (url_params := self.url_params) or not self.has_default_substories:
      updated_url = update_url_query(f"{self.url}/developer.html", url_params)
      logging.info("CUSTOM URL: %s", updated_url)
      return updated_url
    return self.url

  def setup(self, run: Run) -> None:
    test_url = self.prepare_test_url()
    use_developer_url = test_url != self.url
    with run.actions("Setup") as actions:
      actions.show_url(test_url)
      self._setup_wait_until_ready(actions, use_developer_url)
      if use_developer_url:
        self._setup_filter_stories(actions)

  def _setup_wait_until_ready(self, actions, use_developer_url) -> None:
    if use_developer_url:
      wait_js = self.DEVELOPER_READY_JS
    else:
      wait_js = self.READY_JS
    actions.wait_js_condition(wait_js, 0.2, 10)

  def _setup_filter_stories(self, actions) -> None:
    num_enabled = actions.js(
        """
      let benchmarks = arguments[0];
      const list = document.querySelectorAll(".tree li");
      let counter = 0;
      for (const row of list) {
        const name = row.querySelector("label.tree-label").textContent.trim();
        let checked = benchmarks.includes(name);
        const labels = row.querySelectorAll("input[type=checkbox]");
        for (const label of labels) {
          if (checked) {
            label.click()
            counter++;
          }
        }
      }
      return counter
      """,
        arguments=[self._substories])
    assert num_enabled > 0, "No tests were enabled"
    actions.wait(0.1)

  def run(self, run: Run) -> None:
    with run.actions("Running") as actions:
      actions.js("window.benchmarkController.startBenchmark()")
      actions.wait(self.fast_duration)
    with run.actions("Waiting for completion") as actions:
      actions.wait_js_condition(
          """
          return window.benchmarkRunnerClient.results._results != undefined
          """, self.substory_duration / 4, self.slow_duration)


class MotionMark1Benchmark(MotionMarkBenchmark):
  pass
