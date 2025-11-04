# Copyright 2024 The Chromium Authors
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

from __future__ import annotations

from typing import TYPE_CHECKING, Optional

from crossbench import cli_helper
from crossbench.probes.json import JsonResultProbe, JsonResultProbeContext
from crossbench.probes.metric import MetricsMerger
from crossbench.probes.probe import (ProbeConfigParser, ProbeKeyT,
                                     ResultLocation)
from crossbench.probes.results import ProbeResult

if TYPE_CHECKING:
  from crossbench.runner.actions import Actions
  from crossbench.runner.groups.browsers import BrowsersRunGroup
  from crossbench.runner.groups.stories import StoriesRunGroup
  from crossbench.runner.run import Run
  from crossbench.types import Json


def parse_javascript(value: str) -> str:
  # TODO: maybe add more sanity checks
  return cli_helper.parse_non_empty_str(value, name="javascript")


class JSProbe(JsonResultProbe):
  """
  Probe for extracting arbitrary metrics using custom javascript code.
  """
  NAME = "js"
  RESULT_LOCATION = ResultLocation.LOCAL
  IS_GENERAL_PURPOSE = True

  @classmethod
  def config_parser(cls) -> ProbeConfigParser:
    parser = super().config_parser()
    parser.add_argument(
        "setup",
        type=parse_javascript,
        help=(
            "Optional JavaScript code that is run immediately before a story. "
            "This can be used for setting up some JS tracking code or patch "
            "existing code for custom metric tracking."))
    parser.add_argument(
        "js",
        type=parse_javascript,
        required=True,
        help=("Required JavaScript code that is run immediately after "
              "a story has finished. The code must return a JS object with "
              "(nested) metric values (numbers)."))
    return parser

  def __init__(self, js: str, setup: Optional[str] = None) -> None:
    super().__init__()
    self._setup_js = setup
    self._metric_js = js

  @property
  def setup_js(self) -> Optional[str]:
    return self._setup_js

  @property
  def metric_js(self) -> str:
    return self._metric_js

  @property
  def key(self) -> ProbeKeyT:
    return super().key + (
        ("setup_js", self._setup_js),
        ("metric_js", self._metric_js),
    )

  def to_json(self, actions: Actions) -> Json:
    data = actions.js(self._metric_js)
    return cli_helper.parse_non_empty_dict(data, "JS metric data")

  def get_context(self, run: Run) -> JSProbeContext:
    return JSProbeContext(self, run)

  def merge_stories(self, group: StoriesRunGroup) -> ProbeResult:
    merged = MetricsMerger.merge_json_list(
        story_group.results[self].json
        for story_group in group.repetitions_groups)
    return self.write_group_result(group, merged, write_csv=True)

  def merge_browsers(self, group: BrowsersRunGroup) -> ProbeResult:
    return self.merge_browsers_json_list(group).merge(
        self.merge_browsers_csv_list(group))


class JSProbeContext(JsonResultProbeContext[JSProbe]):

  def start(self) -> None:
    if setup_js := self.probe.setup_js:
      with self.run.actions(f"Probe({self.probe.name}) setup") as actions:
        actions.js(setup_js)
