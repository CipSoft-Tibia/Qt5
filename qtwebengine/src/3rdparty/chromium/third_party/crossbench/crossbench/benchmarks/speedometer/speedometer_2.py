# Copyright 2023 The Chromium Authors
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

from __future__ import annotations

from typing import Any, Dict, Tuple

from crossbench.benchmarks.speedometer.speedometer import (
    SpeedometerProbe, SpeedometerProbeContext, SpeedometerStory)
from crossbench.helper import url_helper
from crossbench.parse import ObjectParser


class Speedometer2Probe(SpeedometerProbe):

  def _is_valid_metric_key(self, metric_key: str) -> bool:
    parts = metric_key.split("/")
    if len(parts) == 2:
      return True
    if len(parts) == 1:
      return parts[0] in ("Geomean", "Score")
    return parts[-1] == "total"


class Speedometer2ProbeContext(SpeedometerProbeContext):

  def process_json_data(self, json_data) -> Any:
    json_data = ObjectParser.non_empty_sequence(json_data,
                                                f"{self.probe.name} metrics")
    # Move aggregate scores to the end
    for iteration_data in json_data:
      assert isinstance(iteration_data, dict)
      iteration_data["Mean"] = iteration_data.pop("mean")
      iteration_data["Total"] = iteration_data.pop("total")
      iteration_data["Geomean"] = iteration_data.pop("geomean")
      iteration_data["Score"] = iteration_data.pop("score")
    return json_data


class Speedometer2Story(SpeedometerStory):
  __doc__ = SpeedometerStory.__doc__
  SUBSTORIES: Tuple[str, ...] = (
      "VanillaJS-TodoMVC",
      "Vanilla-ES2015-TodoMVC",
      "Vanilla-ES2015-Babel-Webpack-TodoMVC",
      "React-TodoMVC",
      "React-Redux-TodoMVC",
      "EmberJS-TodoMVC",
      "EmberJS-Debug-TodoMVC",
      "BackboneJS-TodoMVC",
      "AngularJS-TodoMVC",
      "Angular2-TypeScript-TodoMVC",
      "VueJS-TodoMVC",
      "jQuery-TodoMVC",
      "Preact-TodoMVC",
      "Inferno-TodoMVC",
      "Elm-TodoMVC",
      "Flight-TodoMVC",
  )

  @property
  def test_url(self) -> str:
    test_url = f"{self.URL}/InteractiveRunner.html"
    params: Dict[str, str] = self.url_params
    if len(self.substories) == 1:
      params["suite"] = self.substories[0]
    params["startAutomatically"] = "true"
    official_test_url = url_helper.update_url_query(test_url, params)
    return official_test_url
