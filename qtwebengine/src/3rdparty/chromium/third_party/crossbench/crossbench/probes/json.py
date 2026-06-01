# Copyright 2022 The Chromium Authors
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

from __future__ import annotations

import abc
import csv
import json
import logging
from collections import defaultdict
from typing import (TYPE_CHECKING, Any, Callable, Dict, Generic, List, Optional,
                    Tuple, Type, TypeVar, Union)

from tabulate import tabulate

from crossbench.probes import helper
from crossbench.probes.metric import (CSVFormatter, MetricsMerger,
                                      metric_geomean)
from crossbench.probes.probe import Probe, ProbeContext
from crossbench.probes.probe_error import ProbeMissingDataError
from crossbench.probes.results import LocalProbeResult, ProbeResult

if TYPE_CHECKING:
  from crossbench.path import LocalPath
  from crossbench.runner.actions import Actions
  from crossbench.runner.groups.base import RunGroup
  from crossbench.runner.groups.browsers import BrowsersRunGroup
  from crossbench.runner.groups.repetitions import RepetitionsRunGroup
  from crossbench.runner.run import Run
  from crossbench.types import Json

class JsonResultProbe(Probe, metaclass=abc.ABCMeta):
  """
  Abstract Probe that stores a Json result extracted by the `to_json` method

  Tje `to_json` is provided by subclasses. A typical examples includes just
  running a JS script on the page.
  Multiple Json result files for RepetitionsRunGroups are merged with the
  MetricsMerger. Custom merging for other RunGroups can be defined in the
  subclass.
  """

  SORT_KEYS = True

  @property
  def result_path_name(self) -> str:
    return f"{self.name}.json"

  def merge_repetitions(
      self,
      group: RepetitionsRunGroup,
  ) -> ProbeResult:
    merger = MetricsMerger()
    for run in group.runs:
      if self not in run.results:
        raise ProbeMissingDataError(
            f"Probe {self.NAME} produced no data to merge.")
      source_file = run.results[self].json
      assert source_file.is_file(), (
          f"{source_file} from {run} is not a file or doesn't exist.")
      with source_file.open(encoding="utf-8") as f:
        merger.add(json.load(f))
    return self.write_group_result(group, merger, csv_formatter=CSVFormatter)

  def merge_browsers_json_list(self, group: BrowsersRunGroup) -> ProbeResult:
    merged_json: Dict[str, Dict[str, Any]] = {}
    for story_group in group.story_groups:
      browser_result: Dict[str, Any] = {}
      merged_json[story_group.browser.unique_name] = browser_result
      browser_result["info"] = story_group.info
      browser_json_path = story_group.results[self].json
      assert browser_json_path.is_file(), (
          f"{browser_json_path} from {story_group} "
          "is not a file or doesn't exist.")
      with browser_json_path.open(encoding="utf-8") as f:
        browser_result["data"] = json.load(f)
    merged_json_path = group.get_local_probe_result_path(self)
    assert not merged_json_path.exists(), (
        f"Cannot override existing Json result: {merged_json_path}")
    with merged_json_path.open("w", encoding="utf-8") as f:
      json.dump(merged_json, f, indent=2)
      # TODO(375390958): figure out why files aren't fully written to
      # pyfakefs here.
      f.write("\n")
    return LocalProbeResult(json=(merged_json_path,))

  def merge_browsers_csv_list(self, group: BrowsersRunGroup) -> ProbeResult:
    csv_file_list: List[LocalPath] = []
    for story_group in group.story_groups:
      csv_file_list.append(story_group.results[self].csv)
    merged_table = helper.merge_csv(csv_file_list, row_header_len=-1)
    merged_json_path = group.get_local_probe_result_path(self, exists_ok=True)
    merged_csv_path = merged_json_path.with_suffix(".csv")
    assert not merged_csv_path.exists(), (
        f"Cannot override existing CSV result: {merged_csv_path}")
    with merged_csv_path.open("w", newline="", encoding="utf-8") as f:
      csv.writer(f, delimiter="\t").writerows(merged_table)
    return LocalProbeResult(csv=(merged_csv_path,))

  def write_group_result(
      self,
      group: RunGroup,
      merged_data: Union[Dict, List, MetricsMerger],
      csv_formatter: Optional[Type[CSVFormatter]] = CSVFormatter,
      value_fn: Callable[[Any], Any] = metric_geomean) -> ProbeResult:
    merged_json_path = group.get_local_probe_result_path(self)
    with merged_json_path.open("w", encoding="utf-8") as f:
      if isinstance(merged_data, (dict, list)):
        json.dump(merged_data, f, indent=2)
      else:
        json.dump(merged_data.to_json(sort=self.SORT_KEYS), f, indent=2)
      # TODO(375390958): figure out why files aren't fully written to
      # pyfakefs here.
      f.write("\n")
    if not csv_formatter:
      return LocalProbeResult(json=(merged_json_path,))
    if not isinstance(merged_data, MetricsMerger):
      raise ValueError("write_csv is only supported for MetricsMerger, "
                       f"but found {type(merged_data)}'.")
    return self.write_group_csv_result(group, merged_data, merged_json_path,
                                       csv_formatter, value_fn)

  def write_group_csv_result(self, group: RunGroup, merged_data: MetricsMerger,
                             merged_json_path: LocalPath,
                             csv_formatter: Type[CSVFormatter],
                             value_fn: Callable[[Any], Any]) -> ProbeResult:
    merged_csv_path = merged_json_path.with_suffix(".csv")
    assert not merged_csv_path.exists(), (
        f"Cannot override existing CSV result: {merged_csv_path}")
    # Create a CSV table:
    # 0 | info label 0,                                          info_value 0
    #     ...                                                    ...
    # N | info label N,                                          info_value N
    # 0 | metric 0 full path, metric path[0] ... metric path[N], metric 0 value
    #     ...                                                    ...
    # M | metric M full path, ...                                metric M value
    headers: List[Tuple[str, Any]] = []
    for label, info_value in group.info.items():
      headers.append((label, info_value))
    csv_data = csv_formatter(
        merged_data, value_fn, headers=headers, sort=self.SORT_KEYS).table
    with merged_csv_path.open("w", newline="", encoding="utf-8") as f:
      writer = csv.writer(f, delimiter="\t")
      writer.writerows(csv_data)
    return LocalProbeResult(json=(merged_json_path,), csv=(merged_csv_path,))

  LOG_SUMMARY_KEYS = ("label", "browser", "version", "os", "device", "cpu",
                      "runs", "failed runs")

  def _log_result_metrics(self, data: Dict) -> None:
    table: Dict[str, List[str]] = defaultdict(list)
    for browser_result in data.values():
      for info_key in self.LOG_SUMMARY_KEYS:
        table[info_key].append(browser_result["info"][info_key])
      data = browser_result["data"]
      self._extract_result_metrics_table(data, table)
    flattened: List[List[str]] = list(
        [label] + values for label, values in table.items())
    logging.critical(tabulate(flattened, tablefmt="plain"))

  def _extract_result_metrics_table(self, metrics: Dict[str, Any],
                                    table: Dict[str, List[str]]) -> None:
    """Add individual metrics to the table in here.
    Typically you only add score and total values for each benchmark or
    benchmark item."""
    del metrics
    del table


JsonResultProbeT = TypeVar("JsonResultProbeT", bound="JsonResultProbe")


class JsonResultProbeContext(
    ProbeContext[JsonResultProbeT],
    Generic[JsonResultProbeT],
    metaclass=abc.ABCMeta):

  FLATTEN: bool = True

  def __init__(self, probe: JsonResultProbeT, run: Run) -> None:
    super().__init__(probe, run)
    self._json_data: Json = None

  @property
  def probe(self) -> JsonResultProbeT:
    return super().probe

  @abc.abstractmethod
  def to_json(self, actions: Actions) -> Json:
    """
    Override in subclasses.
    Returns json-serializable data.
    """
    return None

  def start(self) -> None:
    pass

  def stop(self) -> None:
    self._json_data = self.extract_json(self.run)

  def teardown(self) -> ProbeResult:
    if self._json_data is None:
      return self.empty_result()
    self._json_data = self.process_json_data(self._json_data)
    return self.write_json(self.run, self._json_data)

  def extract_json(self, run: Run) -> Json:
    with run.actions(f"Extracting Probe({self.probe.name})") as actions:
      json_data = self.to_json(actions)
      assert json_data is not None, (
          f"Probe({self.probe.name}) produced no data")
      return json_data

  def write_json(self, run: Run, json_data: Json) -> ProbeResult:
    flattened_file = None
    with run.actions(f"Writing Probe({self.probe.name})"):
      assert json_data is not None, (
          f"Probe({self.probe.name}) produced no Json data.")
      raw_file = self.local_result_path
      if self.FLATTEN:
        raw_file = raw_file.with_suffix(".json.nested")
        flattened_file = self.local_result_path
        flat_json_data = self.flatten_json_data(json_data)
        with flattened_file.open("w", encoding="utf-8") as f:
          json.dump(flat_json_data, f, indent=2)
          # TODO(375390958): figure out why files aren't fully written to
          # pyfakefs here.
          f.write("\n")
      with raw_file.open("w", encoding="utf-8") as f:
        json.dump(json_data, f, indent=2)
        # TODO(375390958): figure out why files aren't fully written to
        # pyfakefs here.
        f.write("\n")
    if flattened_file:
      return LocalProbeResult(json=(flattened_file,), file=(raw_file,))
    return LocalProbeResult(json=(raw_file,))

  def process_json_data(self, json_data: Json) -> Json:
    return json_data

  def flatten_json_data(self, json_data: Any) -> Json:
    return helper.Flatten(json_data).data
