# Copyright 2023 The Chromium Authors
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

from __future__ import annotations

import logging
from typing import TYPE_CHECKING, Tuple

from crossbench import helper

if TYPE_CHECKING:
  from crossbench.runner.run import Run

from .speedometer import SpeedometerStory


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
