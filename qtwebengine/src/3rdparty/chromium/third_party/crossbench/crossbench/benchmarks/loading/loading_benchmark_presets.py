# Copyright 2024 The Chromium Authors
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

from __future__ import annotations

import abc
import argparse
import logging
import pandas as pd
import numpy as np
from tabulate import tabulate
from typing import TYPE_CHECKING, Optional, Sequence, Tuple

from crossbench import path as pth
from crossbench.probes.probe import Probe, ProbeContext
from crossbench.probes.results import (EmptyProbeResult, ProbeResult)
from crossbench.probes.trace_processor.trace_processor import TraceProcessorProbe
from crossbench.benchmarks.base import BenchmarkProbeMixin
from crossbench.benchmarks.loading.config.pages import PagesConfig
from crossbench.benchmarks.loading.loading_benchmark import (LoadingPageFilter,
                                                             PageLoadBenchmark)
from crossbench.flags.base import Flags

if TYPE_CHECKING:
  from crossbench.benchmarks.loading.page import Page
  from crossbench.browsers.browser import Browser
  from crossbench.runner.groups import BrowsersRunGroup
  from crossbench.runner.runner import Run

CONFIG_DIR = pth.LocalPath(__file__).parents[3] / "config"
LOADING_DIR = CONFIG_DIR / "benchmark" / "loading"


class PresetLoadingPageFilter(LoadingPageFilter):
  """Page Load benchmark for phone/tablet."""
  CAN_COMBINE_STORIES: bool = False

  @classmethod
  def add_page_config_parser(cls, parser: argparse.ArgumentParser) -> None:
    pass

  @classmethod
  def default_stories(cls) -> Tuple[Page, ...]:
    return cls.all_stories()

  @classmethod
  def all_stories(cls) -> Tuple[Page, ...]:
    return ()


class PresetPageLoadBenchmarkProbe(BenchmarkProbeMixin, Probe):
  IS_GENERAL_PURPOSE = False
  NAME = "preset_page_load_benchmark_probe"

  def get_context(self,
                  run: Run) -> Optional[PresetPageLoadBenchmarkProbeContext]:
    return PresetPageLoadBenchmarkProbeContext(self, run)

  def log_browsers_result(self, group: BrowsersRunGroup) -> None:
    logging.info("-" * 80)
    logging.critical("Loading results:")
    logging.info("- " * 40)
    logging.critical(
        tabulate(
            pd.read_csv(
                group.get_local_probe_result_path(self).with_suffix(".csv")),
            headers="keys",
            tablefmt="plain"))

  def merge_browsers(self, group: BrowsersRunGroup) -> ProbeResult:
    csv_file = group.get_local_probe_result_path(self).with_suffix(".csv")
    self._compute_score(group).to_csv(csv_file)
    return ProbeResult(csv=(csv_file,))

  def _compute_score(self, group: BrowsersRunGroup) -> pd.DataFrame:
    df = pd.read_csv(group.results.get_by_name(TraceProcessorProbe.NAME).csv)
    df = df.groupby(["cb_browser",
                     "cb_story"])["score"].mean().reset_index().pivot(
                         columns=["cb_story"],
                         index=["cb_browser"],
                         values=["score"])
    df = df.droplevel(0, axis=1)
    df["TOTAL_SCORE"] = np.exp(np.log(df).mean(axis=1))
    df.index.rename("browser", inplace=True)
    return df.reindex(
        columns=(['TOTAL_SCORE'] +
                 sorted(list(c for c in df.columns if c != 'TOTAL_SCORE'))))


class PresetPageLoadBenchmarkProbeContext(
    ProbeContext[PresetPageLoadBenchmarkProbe]):

  def start(self) -> None:
    pass

  def start_story_run(self) -> None:
    self.browser.performance_mark(
        self.runner,
        f"PresetPageLoadBenchmark/{self.probe.benchmark.NAME}/{self.run.story.name}"
    )

  def stop(self) -> None:
    pass

  def teardown(self) -> ProbeResult:
    return EmptyProbeResult()


class PresetPageLoadBenchmark(PageLoadBenchmark, metaclass=abc.ABCMeta):
  STORY_FILTER_CLS = PresetLoadingPageFilter
  PROBES = (PresetPageLoadBenchmarkProbe,)

  @classmethod
  def cli_description(cls) -> str:
    return cls.__doc__.strip()

  @classmethod
  def requires_separate(cls, args: argparse.Namespace):
    # Perfetto metrics used in the benchmark require a separate Perfetto
    # session for each run.
    return True

  @classmethod
  def default_probe_config_path(cls) -> pth.LocalPath:
    return pth.LocalPath(LOADING_DIR) / "probe_config.hjson"

  @classmethod
  @abc.abstractmethod
  def default_network_config_path(cls) -> pth.LocalPath:
    pass

  @classmethod
  @abc.abstractmethod
  def default_pages_config_path(cls) -> pth.LocalPath:
    pass

  @classmethod
  def get_pages_config(
      cls, args: Optional[argparse.Namespace] = None) -> PagesConfig:
    return PagesConfig.parse(cls.default_pages_config_path())

  @classmethod
  def all_story_names(cls) -> Sequence[str]:
    return tuple(page.label for page in cls.get_pages_config().pages)

class PageLoadTabletBenchmark(PresetPageLoadBenchmark):
  """Page Load benchmark for tablet.
  """
  NAME = "loading-tablet"

  @classmethod
  def default_pages_config_path(cls) -> pth.LocalPath:
    return pth.LocalPath(LOADING_DIR) / "page_config_tablet.hjson"

  @classmethod
  def default_network_config_path(cls) -> pth.LocalPath:
    return pth.LocalPath(LOADING_DIR) / "network_config_tablet.hjson"

  @classmethod
  def aliases(cls) -> Tuple[str, ...]:
    return ("load-tablet", "ld-tablet")

  @classmethod
  def extra_flags(cls, browser: Browser) -> Flags:
    assert browser.attributes.is_chromium_based
    return Flags(["--request-desktop-sites"])


class PageLoadPhoneBenchmark(PresetPageLoadBenchmark):
  """Page Load benchmark for phones.
  """
  NAME = "loading-phone"

  @classmethod
  def default_pages_config_path(cls) -> pth.LocalPath:
    return pth.LocalPath(LOADING_DIR) / "page_config_phone.hjson"

  @classmethod
  def default_network_config_path(cls) -> pth.LocalPath:
    return pth.LocalPath(LOADING_DIR) / "network_config_phone.hjson"

  @classmethod
  def aliases(cls) -> Tuple[str, ...]:
    return ("load-phone", "ld-phone")
