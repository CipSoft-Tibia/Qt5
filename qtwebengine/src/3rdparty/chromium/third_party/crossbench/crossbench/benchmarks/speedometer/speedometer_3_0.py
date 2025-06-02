# Copyright 2023 The Chromium Authors
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

from __future__ import annotations

import argparse
import datetime as dt
import enum
import logging
from typing import (TYPE_CHECKING, Any, Dict, List, Optional, Sequence, Tuple,
                    Type)

from crossbench import cli_helper, compat, helper

if TYPE_CHECKING:
  from crossbench.runner.run import Run

from .speedometer import (ProbeClsTupleT, SpeedometerBenchmark,
                          SpeedometerBenchmarkStoryFilter, SpeedometerProbe,
                          SpeedometerStory)


class Speedometer30Probe(SpeedometerProbe):
  """
  Speedometer3-specific probe (compatible with v3.0).
  Extracts all speedometer times and scores.
  """
  NAME: str = "speedometer_3.0"


@enum.unique
class MeasurementMethod(compat.StrEnumWithHelp):
  RAF = ("raf", "requestAnimationFrame-based measurement")
  TIMER = ("timer", "setTimeout-based measurement")


def to_ms(duration: dt.timedelta) -> int:
  return int(round(duration.total_seconds() * 1000))


class Speedometer30Story(SpeedometerStory):
  __doc__ = SpeedometerStory.__doc__
  NAME: str = "speedometer_3.0"
  PROBES: ProbeClsTupleT = (Speedometer30Probe,)
  # TODO: Update once public version is available
  URL: str = "https://sp3-alpha-testing.netlify.app/"
  URL_LOCAL: str = "http://127.0.0.1:7000"
  SUBSTORIES = (
      "TodoMVC-JavaScript-ES5",
      "TodoMVC-JavaScript-ES6-Webpack",
      "TodoMVC-WebComponents",
      "TodoMVC-React",
      "TodoMVC-React-Complex-DOM",
      "TodoMVC-React-Redux",
      "TodoMVC-Backbone",
      "TodoMVC-Angular",
      "TodoMVC-Vue",
      "TodoMVC-jQuery",
      "TodoMVC-Preact",
      "TodoMVC-Svelte",
      "TodoMVC-Lit",
      "TodoMVC-JavaScript-ES5-Complex-DOM",
      "TodoMVC-JavaScript-ES6-Webpack-Complex-DOM",
      "TodoMVC-WebComponents-Complex-DOM",
      "TodoMVC-React-Redux-Complex-DOM",
      "TodoMVC-Backbone-Complex-DOM",
      "TodoMVC-Angular-Complex-DOM",
      "TodoMVC-Vue-Complex-DOM",
      "TodoMVC-jQuery-Complex-DOM",
      "TodoMVC-Preact-Complex-DOM",
      "TodoMVC-Svelte-Complex-DOM",
      "TodoMVC-Lit-Complex-DOM",
      "NewsSite-Next",
      "NewsSite-Nuxt",
      "Editor-CodeMirror",
      "Editor-TipTap",
      "Charts-observable-plot",
      "Charts-chartjs",
      "React-Stockcharts-SVG",
      "Perf-Dashboard",
  )

  def __init__(self,
               substories: Sequence[str] = (),
               iterations: Optional[int] = None,
               sync_wait: Optional[dt.timedelta] = None,
               sync_warmup: Optional[dt.timedelta] = None,
               measurement_method: Optional[MeasurementMethod] = None,
               url: Optional[str] = None):
    self._sync_wait = cli_helper.Duration.parse_zero(
        sync_wait or dt.timedelta(0), "sync_wait")
    self._sync_warmup = cli_helper.Duration.parse_zero(
        sync_warmup or dt.timedelta(0), "sync_warmup")
    self._measurement_method: MeasurementMethod = (
        measurement_method or MeasurementMethod.RAF)
    super().__init__(url=url, substories=substories, iterations=iterations)

  @property
  def sync_wait(self) -> dt.timedelta:
    return self._sync_wait

  @property
  def sync_warmup(self) -> dt.timedelta:
    return self._sync_warmup

  @property
  def measurement_method(self) -> MeasurementMethod:
    return self._measurement_method

  @property
  def url_params(self) -> Dict[str, str]:
    url_params = super().url_params
    if self.sync_wait:
      url_params["waitBeforeSync"] = str(to_ms(self.sync_wait))
    if self.sync_warmup:
      url_params["warmupBeforeSync"] = str(to_ms(self.sync_warmup))
    if self.measurement_method != MeasurementMethod.RAF:
      url_params["measurementMethod"] = str(self.measurement_method)
    return url_params

  def log_run_test_url(self, run: Run) -> None:
    del run
    params = self.url_params
    params["suites"] = ",".join(self.substories)
    params["developerMode"] = "true"
    params["startAutomatically"] = "true"
    official_test_url = helper.update_url_query(self.URL, params)
    logging.info("STORY PUBLIC TEST URL: %s", official_test_url)


class Speedometer3BenchmarkStoryFilter(SpeedometerBenchmarkStoryFilter):
  __doc__ = SpeedometerBenchmarkStoryFilter.__doc__

  @classmethod
  def add_cli_parser(
      cls, parser: argparse.ArgumentParser) -> argparse.ArgumentParser:
    parser = super().add_cli_parser(parser)
    parser.add_argument(
        "--sync-wait",
        default=dt.timedelta(0),
        type=cli_helper.Duration.parse_zero,
        help="Add a custom wait timeout before each sync step.")
    parser.add_argument(
        "--sync-warmup",
        default=dt.timedelta(0),
        type=cli_helper.Duration.parse_zero,
        help="Run a warmup loop for the given duration before each sync step.")

    measurement_method_group = parser.add_argument_group(
        "Measurement Method Option")
    measurement_method_group = parser.add_mutually_exclusive_group()
    measurement_method_group.add_argument(
        "--raf",
        dest="measurement_method",
        default=MeasurementMethod.RAF,
        const=MeasurementMethod.RAF,
        action="store_const",
        help=("Use the default requestAnimationFrame-based approach "
              "for async time measurement."))
    measurement_method_group.add_argument(
        "--timer",
        dest="measurement_method",
        const=MeasurementMethod.TIMER,
        action="store_const",
        help=("Use the 'classical' setTimeout-based approach "
              "for async time measurement. "
              "This might omit measuring some async work."))
    return parser

  @classmethod
  def kwargs_from_cli(cls, args: argparse.Namespace) -> Dict[str, Any]:
    kwargs = super().kwargs_from_cli(args)
    kwargs["iterations"] = args.iterations
    kwargs["measurement_method"] = args.measurement_method
    kwargs["sync_wait"] = args.sync_wait
    kwargs["sync_warmup"] = args.sync_warmup
    return kwargs

  def __init__(self,
               story_cls: Type[SpeedometerStory],
               patterns: Sequence[str],
               separate: bool = False,
               url: Optional[str] = None,
               iterations: Optional[int] = None,
               measurement_method: Optional[MeasurementMethod] = None,
               sync_wait: Optional[dt.timedelta] = None,
               sync_warmup: Optional[dt.timedelta] = None):
    self.measurement_method = measurement_method
    self.sync_wait = sync_wait
    self.sync_warmup = sync_warmup
    assert issubclass(story_cls, Speedometer30Story)
    super().__init__(story_cls, patterns, separate, url, iterations=iterations)

  def create_stories_from_names(self, names: List[str],
                                separate: bool) -> Sequence[SpeedometerStory]:
    return self.story_cls.from_names(
        names,
        separate=separate,
        url=self.url,
        iterations=self.iterations,
        measurement_method=self.measurement_method,
        sync_wait=self.sync_wait,
        sync_warmup=self.sync_warmup)


class Speedometer30Benchmark(SpeedometerBenchmark):
  """
  Benchmark runner for Speedometer 3.0
  """
  NAME: str = "speedometer_3.0"
  DEFAULT_STORY_CLS = Speedometer30Story
  STORY_FILTER_CLS = Speedometer3BenchmarkStoryFilter

  @classmethod
  def version(cls) -> Tuple[int, ...]:
    return (3, 0)

  @classmethod
  def aliases(cls) -> Tuple[str, ...]:
    return ("sp3", "speedometer_3") + super().aliases()
