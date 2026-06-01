# Copyright 2022 The Chromium Authors
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

from __future__ import annotations

import abc
import argparse
import datetime as dt
import logging
from typing import (TYPE_CHECKING, Any, Dict, List, Optional, Sequence, Tuple,
                    Type)

from crossbench.benchmarks.base import PressBenchmarkStoryFilter
from crossbench.benchmarks.jetstream.jetstream import (JetStreamBenchmark,
                                                       JetStreamProbe,
                                                       JetStreamProbeContext)
from crossbench.helper import url_helper
from crossbench.parse import NumberParser
from crossbench.stories.press_benchmark import PressBenchmarkStory

if TYPE_CHECKING:
  from crossbench.runner.run import Run


class JetStream2Probe(JetStreamProbe, metaclass=abc.ABCMeta):
  """
  JetStream2-specific Probe.
  Extracts all JetStream2 times and scores.
  """


class JetStream2ProbeContext(JetStreamProbeContext):
  pass


class JetStream2Story(PressBenchmarkStory, metaclass=abc.ABCMeta):
  URL_LOCAL: str = "http://localhost:8000/"
  SUBSTORIES: Tuple[str, ...] = (
      "WSL",
      "UniPoker",
      "uglify-js-wtb",
      "typescript",
      "tsf-wasm",
      "tagcloud-SP",
      "string-unpack-code-SP",
      "stanford-crypto-sha256",
      "stanford-crypto-pbkdf2",
      "stanford-crypto-aes",
      "splay",
      "segmentation",
      "richards-wasm",
      "richards",
      "regexp",
      "regex-dna-SP",
      "raytrace",
      "quicksort-wasm",
      "prepack-wtb",
      "pdfjs",
      "OfflineAssembler",
      "octane-zlib",
      "octane-code-load",
      "navier-stokes",
      "n-body-SP",
      "multi-inspector-code-load",
      "ML",
      "mandreel",
      "lebab-wtb",
      "json-stringify-inspector",
      "json-parse-inspector",
      "jshint-wtb",
      "HashSet-wasm",
      "hash-map",
      "gcc-loops-wasm",
      "gbemu",
      "gaussian-blur",
      "float-mm.c",
      "FlightPlanner",
      "first-inspector-code-load",
      "espree-wtb",
      "earley-boyer",
      "delta-blue",
      "date-format-xparb-SP",
      "date-format-tofte-SP",
      "crypto-sha1-SP",
      "crypto-md5-SP",
      "crypto-aes-SP",
      "crypto",
      "coffeescript-wtb",
      "chai-wtb",
      "cdjs",
      "Box2D",
      "bomb-workers",
      "Basic",
      "base64-SP",
      "babylon-wtb",
      "Babylon",
      "async-fs",
      "Air",
      "ai-astar",
      "acorn-wtb",
      "3d-raytrace-SP",
      "3d-cube-SP",
  )

  def __init__(self,
               substories: Sequence[str] = (),
               iterations: Optional[int] = None,
               url: Optional[str] = None):
    self._iterations: Optional[int] = iterations
    if iterations is not None:
      self._iterations = NumberParser.positive_int(
          self._iterations, "iteration count", parse_str=False)
    super().__init__(url=url, substories=substories)

  @property
  def substory_duration(self) -> dt.timedelta:
    return dt.timedelta(seconds=2)

  @property
  def iterations(self) -> Optional[int]:
    return self._iterations

  @property
  def url_params(self) -> Dict[str, str]:
    params: Dict[str, str] = {}
    if self.iterations:
      params["iterationCount"] = str(self.iterations)
    return params

  def get_run_url(self, run: Run) -> str:
    url = super().get_run_url(run)
    url = url_helper.update_url_query(url, self.url_params)
    if url != self.url:
      logging.info("CUSTOM URL: %s", url)
    return url

  def setup(self, run: Run) -> None:
    with run.actions("Setup") as actions:
      actions.show_url(self.get_run_url(run))
      if self._substories != self.SUBSTORIES:
        actions.wait_js_condition(("return JetStream && JetStream.benchmarks "
                                   "&& JetStream.benchmarks.length > 0;"), 0.1,
                                  10)
        actions.js(
            """
        let benchmarks = arguments[0];
        JetStream.benchmarks = JetStream.benchmarks.filter(
            benchmark => benchmarks.includes(benchmark.name));
        """,
            arguments=[self._substories])
      actions.wait_js_condition(
          """
        return document.querySelectorAll("#results>.benchmark").length > 0;
      """, 1, self.duration + dt.timedelta(seconds=30))

  def run(self, run: Run) -> None:
    with run.actions("Running") as actions:
      actions.js("JetStream.start()")
      actions.wait(self.fast_duration)
    with run.actions("Waiting for completion") as actions:
      actions.wait_js_condition(
          """
        let summaryElement = document.getElementById("result-summary");
        return (summaryElement.classList.contains("done"));
        """,
          0.5,
          self.slow_duration,
          delay=self.substory_duration)


ProbeClsTupleT = Tuple[Type[JetStream2Probe], ...]


class JetStream2BenchmarkStoryFilter(PressBenchmarkStoryFilter):
  __doc__ = PressBenchmarkStoryFilter.__doc__

  @classmethod
  def add_cli_parser(
      cls, parser: argparse.ArgumentParser) -> argparse.ArgumentParser:
    parser = super().add_cli_parser(parser)
    parser.add_argument(
        "--iterations",
        "--iteration-count",
        default=None,
        type=NumberParser.positive_int,
        help="Number of iterations each JetStream subtest is run "
        "within the same session. \n"
        "Note: --repetitions restarts the whole benchmark, --iterations runs "
        "the same test tests n-times within the same session without the setup "
        "overhead of starting up a whole new browser. \n"
        "This option is not supported on the official benchmark "
        "before version 3.0.")
    return parser

  @classmethod
  def kwargs_from_cli(cls, args: argparse.Namespace) -> Dict[str, Any]:
    kwargs = super().kwargs_from_cli(args)
    kwargs["iterations"] = args.iterations
    return kwargs

  def __init__(self,
               story_cls: Type[JetStream2Story],
               patterns: Sequence[str],
               separate: bool = False,
               url: Optional[str] = None,
               iterations: Optional[int] = None):
    self.iterations = iterations
    assert issubclass(story_cls, JetStream2Story)
    super().__init__(story_cls, patterns, separate, url)

  def create_stories_from_names(self, names: List[str],
                                separate: bool) -> Sequence[JetStream2Story]:
    return self.story_cls.from_names(
        names, separate=separate, url=self.url, iterations=self.iterations)


class JetStream2Benchmark(JetStreamBenchmark):
  STORY_FILTER_CLS = JetStream2BenchmarkStoryFilter
