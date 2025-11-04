# Copyright 2024 The Chromium Authors
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

from __future__ import annotations

import collections
import json
import logging
import zipfile
import pandas as pd

from perfetto.batch_trace_processor.api import BatchTraceProcessor, BatchTraceProcessorConfig
from perfetto.trace_processor.api import TraceProcessorConfig, TraceProcessor
from perfetto.trace_uri_resolver.resolver import TraceUriResolver
from perfetto.trace_uri_resolver.path import PathUriResolver
from perfetto.trace_uri_resolver.registry import ResolverRegistry
from google.protobuf.json_format import MessageToJson
from typing import IO, TYPE_CHECKING, Any, Iterable, List, Optional, Tuple, Union, Dict
from urllib.request import urlretrieve

from crossbench import cli_helper
from crossbench import path as pth
from crossbench.browsers.browser_helper import BROWSERS_CACHE
from crossbench.probes import metric as cb_metric
from crossbench.probes.probe import Probe, ProbeConfigParser, ProbeContext
from crossbench.probes.results import LocalProbeResult, ProbeResult

if TYPE_CHECKING:
  from crossbench.runner.groups.browsers import BrowsersRunGroup
  from crossbench.runner.run import Run
  from crossbench.env import HostEnvironment
  from crossbench.types import JsonDict


_QUERIES_DIR = pth.LocalPath(__file__).parent / "queries"
_MODULES_DIR = pth.LocalPath(__file__).parent / "modules/ext"


class CrossbenchTraceUriResolver(TraceUriResolver):
  PREFIX = 'crossbench'

  def __init__(self, traces: Union[Iterable[Run] | TraceProcessorProbeContext]):

    def metadata(run: Run) -> Dict[str, str]:
      return {
          "cb_browser": run.browser.unique_name,
          "cb_story": run.story.name,
          "cb_temperature": run.temperature,
          "cb_run": str(run.repetition)
      }

    if isinstance(traces, TraceProcessorProbeContext):
      self._resolved = [
          TraceUriResolver.Result(
              trace=str(traces.merged_trace_path.absolute()),
              metadata=metadata(traces.run))
      ]
    else:
      self._resolved = [
          TraceUriResolver.Result(
              trace=str(
                  run.results.get_by_name(
                      TraceProcessorProbe.NAME).trace.absolute()),
              metadata=metadata(run)) for run in traces
      ]

  def resolve(self) -> List['TraceUriResolver.Result']:
    return self._resolved

class TraceProcessorProbe(Probe):
  """
  Trace processor probe.
  """

  NAME = "trace_processor"

  @classmethod
  def config_parser(cls) -> ProbeConfigParser:
    parser = super().config_parser()
    parser.add_argument(
        "batch",
        type=bool,
        required=False,
        default=False,
        help="Run queries in batch mode when all the test runs are done. This "
        "can considerably reduce the run time at the expense of higher "
        "memory usage (all traces will be loaded into memory at the same "
        "time)")
    parser.add_argument(
        "metrics",
        type=str,
        is_list=True,
        default=tuple(),
        help="Name of metric to be run (can be any metric from Perfetto)")
    parser.add_argument(
        "queries",
        type=str,
        is_list=True,
        default=tuple(),
        help="Name of query to be run (under probes/trace_processor/queries)")
    parser.add_argument(
        "trace_processor_bin",
        type=cli_helper.parse_local_binary_path,
        required=False,
        help="Path to the trace_processor binary")
    return parser

  def __init__(self,
               batch: bool,
               metrics: Iterable[str],
               queries: Iterable[str],
               trace_processor_bin: Optional[pth.LocalPath] = None):
    super().__init__()
    self._batch = batch
    self._metrics = tuple(metrics)
    self._queries = tuple(queries)
    self._trace_processor_bin: Optional[pth.LocalPath] = None
    if trace_processor_bin:
      self._trace_processor_bin = cli_helper.parse_local_binary_path(
          trace_processor_bin, "trace_processor")

  @property
  def batch(self) -> bool:
    return self._batch

  @property
  def metrics(self) -> Tuple[str, ...]:
    return self._metrics

  @property
  def queries(self) -> Tuple[str, ...]:
    return self._queries

  @property
  def tp_config(self) -> TraceProcessorConfig:
    extra_flags = [
        "--add-sql-module",
        _MODULES_DIR,
    ]

    return TraceProcessorConfig(
        bin_path=self._trace_processor_bin,
        resolver_registry=ResolverRegistry(
            resolvers=[CrossbenchTraceUriResolver, PathUriResolver]),
        extra_flags=extra_flags)

  def get_context(self, run: Run) -> TraceProcessorProbeContext:
    return TraceProcessorProbeContext(self, run)

  def validate_env(self, env: HostEnvironment) -> None:
    super().validate_env(env)
    self._check_sql()

  def _check_sql(self) -> None:
    """
    Runs all metrics and queries on an empty trace. This will ensure that they
    are correctly defined in trace processor.
    """
    with TraceProcessor(trace='/dev/null', config=self.tp_config) as tp:
      for metric in self.metrics:
        tp.metric([metric])
      for query in self.queries:
        query_path = _QUERIES_DIR / f"{query}.sql"
        tp.query(query_path.read_text())

  def _add_cb_columns(self, df: pd.DataFrame, run: Run) -> pd.DataFrame:
    df["cb_browser"] = run.browser.unique_name
    df["cb_story"] = run.story.name
    df["cb_temperature"] = run.temperature
    df["cb_run"] = run.repetition
    return df

  def _aggregate_results_by_query(
      self, runs: Iterable[Run]) -> Dict[str, pd.DataFrame]:
    res: Dict[str, pd.DataFrame] = {}
    for run in runs:
      for file in run.results.get(self).csv_list:
        df = pd.read_csv(file)
        df = self._add_cb_columns(df, run)
        if file.stem in res:
          res[file.stem] = pd.concat([res[file.stem], df])
        else:
          res[file.stem] = df

    return res

  def _merge_json(self, runs: Iterable[Run]) -> Dict[str, JsonDict]:
    merged_metrics = collections.defaultdict(cb_metric.MetricsMerger)
    for run in runs:
      for file_path in run.results[self].json_list:
        with file_path.open() as json_file:
          merged_metrics[file_path.stem].add(json.load(json_file))

    return {
        metric_name: merged.to_json()
        for metric_name, merged in merged_metrics.items()
    }

  def merge_browsers(self, group: BrowsersRunGroup) -> ProbeResult:
    group_dir = group.get_local_probe_result_path(self)
    group_dir.mkdir()

    if not self.batch:
      csv_files = []
      json_files = []
      for query, df in self._aggregate_results_by_query(group.runs).items():
        csv_file = group_dir / f"{pth.safe_filename(query)}.csv"
        df.to_csv(path_or_buf=csv_file, index=False)
        csv_files.append(csv_file)
      for metric, data in self._merge_json(group.runs).items():
        json_file = group_dir / f"{pth.safe_filename(metric)}.json"
        with json_file.open("x") as f:
          json.dump(data, f, indent=4)
        json_files.append(json_file)
      return LocalProbeResult(csv=csv_files, json=json_files)

    btp_config = BatchTraceProcessorConfig(tp_config=self.tp_config)

    with BatchTraceProcessor(
        traces=CrossbenchTraceUriResolver(group.runs),
        config=btp_config) as btp:

      def run_query(query: str):
        query_path = _QUERIES_DIR / f"{query}.sql"
        csv_file = group_dir / f"{pth.safe_filename(query)}.csv"
        btp.query_and_flatten(query_path.read_text()).to_csv(
            path_or_buf=csv_file, index=False)
        return csv_file

      csv_files = list(map(run_query, self.queries))

      def run_metric(metric: str):
        json_file = group_dir / f"{pth.safe_filename(metric)}.json"
        protos = btp.metric([metric])
        with json_file.open("x") as f:
          for p in protos:
            f.write(MessageToJson(p))
        return json_file

      json_files = list(map(run_metric, self.metrics))

    return LocalProbeResult(csv=csv_files, json=json_files)

  def log_browsers_result(self, group: BrowsersRunGroup) -> None:
    logging.info("-" * 80)
    logging.critical("TraceProcessor merged traces:")
    for run in group.runs:
      logging.critical("  - %s", run.results[self].trace)

class TraceProcessorProbeContext(ProbeContext[TraceProcessorProbe]):

  def __init__(self, probe: TraceProcessorProbe, run: Run) -> None:
    super().__init__(probe, run)

  def get_default_result_path(self) -> pth.RemotePath:
    result_dir = super().get_default_result_path()
    self.runner_platform.mkdir(result_dir)
    return result_dir

  def setup(self) -> None:
    pass

  def start(self) -> None:
    pass

  def stop(self) -> None:
    pass

  def teardown(self) -> ProbeResult:
    with self.run.actions("TRACE_PROCESSOR: Merging trace files", verbose=True):
      with zipfile.ZipFile(self.merged_trace_path, "w") as zip_file:
        for f in self.run.results.all_traces():
          zip_file.write(f, arcname=f.relative_to(self.run.out_dir))

    if self.probe.batch:
      return LocalProbeResult(
          trace=[self.merged_trace_path], file=[self.merged_trace_path])

    with TraceProcessor(
        trace=CrossbenchTraceUriResolver(self),
        config=self.probe.tp_config) as tp:
      csv_files = self._run_queries(tp)
      json_files = self._run_metrics(tp)

    return LocalProbeResult(
        trace=[self.merged_trace_path], csv=csv_files, json=json_files)

  def _run_queries(self, tp: TraceProcessor) -> List[pth.LocalPath]:

    def run_query(query: str):
      query_path = _QUERIES_DIR / f"{query}.sql"
      csv_file = self.local_result_path / f"{pth.safe_filename(query)}.csv"
      tp.query(query_path.read_text()).as_pandas_dataframe().to_csv(
          path_or_buf=csv_file, index=False)
      return csv_file

    with self.run.actions("TRACE_PROCESSOR: Running queries", verbose=True):
      return list(map(run_query, self.probe.queries))

  def _run_metrics(self, tp: TraceProcessor) -> List[pth.LocalPath]:

    def run_metric(metric: str):
      json_file = self.local_result_path / f"{pth.safe_filename(metric)}.json"
      proto = tp.metric([metric])
      with json_file.open("x") as f:
        f.write(MessageToJson(proto))
      return json_file

    with self.run.actions("TRACE_PROCESSOR: Running metrics", verbose=True):
      return list(map(run_metric, self.probe.metrics))

  @property
  def merged_trace_path(self):
    return self.local_result_path / "merged_trace.zip"
