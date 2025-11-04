# Copyright 2023 The Chromium Authors
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

from __future__ import annotations

import logging
from typing import TYPE_CHECKING, Any, Tuple

from crossbench import helper
from crossbench.benchmarks.speedometer.speedometer import (SpeedometerProbe,
                                                           SpeedometerStory)

if TYPE_CHECKING:
  from crossbench.runner.run import Run


class Speedometer2Probe(SpeedometerProbe):

  def _valid_metric_key(self, metric_key: str) -> bool:
    parts = metric_key.split("/")
    if len(parts) == 2:
      return True
    if len(parts) == 1:
      return parts[0] in ("Geomean", "Score")
    return parts[-1] == "total"

  def process_json_data(self, json_data) -> Any:
    # Move aggregate scores to the end
    for iteration_data in json_data:
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

  def log_run_test_url(self, run: Run) -> None:
    test_url = f"{self.URL}/InteractiveRunner.html"
    params = self.url_params
    if len(self.substories) == 1:
      params["suite"] = self.substories[0]
    params["startAutomatically"] = "true"
    official_test_url = helper.update_url_query(test_url, params)
    logging.info("STORY PUBLIC TEST URL: %s", official_test_url)
