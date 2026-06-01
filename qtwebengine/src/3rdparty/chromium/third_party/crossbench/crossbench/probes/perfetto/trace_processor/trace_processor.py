# Copyright 2024 The Chromium Authors
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

from __future__ import annotations

import collections
import dataclasses
import json
import logging
import zipfile
from typing import (TYPE_CHECKING, Dict, Iterable, List, Optional, Tuple, Type,
                    Union)

import pandas as pd
from google.protobuf.json_format import MessageToJson
from perfetto.batch_trace_processor.api import (BatchTraceProcessor,
                                                BatchTraceProcessorConfig)
from perfetto.trace_processor.api import TraceProcessor, TraceProcessorConfig
from perfetto.trace_uri_resolver.path import PathUriResolver
from perfetto.trace_uri_resolver.registry import ResolverRegistry
from perfetto.trace_uri_resolver.resolver import TraceUriResolver

from crossbench import path as pth
from crossbench.config import ConfigObject, ConfigParser
from crossbench.parse import ObjectParser, PathParser
from crossbench.probes.metric import MetricsMerger
from crossbench.probes.probe import Probe, ProbeConfigParser, ProbeContext
from crossbench.probes.results import LocalProbeResult, ProbeResult

if TYPE_CHECKING:
  from crossbench.env import HostEnvironment
  from crossbench.runner.groups.browsers import BrowsersRunGroup
  from crossbench.runner.run import Run
  from crossbench.types import JsonDict

_QUERIES_DIR = pth.LocalPath(__file__).parent / "queries"
_MODULES_DIR = pth.LocalPath(__file__).parent / "modules/ext"


@dataclasses.dataclass(frozen=True)
class TraceProcessorQueryConfig(ConfigObject):
  name: str
  sql: str

  @classmethod
  def parse_str(cls, value: str) -> TraceProcessorQueryConfig:
    name = ObjectParser.safe_filename(value)
    sql_path = PathParser.existing_file_path(_QUERIES_DIR / f"{value}.sql",
                                             "sql query")
    sql = sql_path.read_text(encoding="utf-8")
    return cls(name=name, sql=sql)

  @classmethod
  def parse_dict(cls, config: Dict) -> TraceProcessorQueryConfig:
    return cls.config_parser().parse(config)

  @classmethod
  def config_parser(cls) -> ConfigParser[TraceProcessorQueryConfig]:
    parser = ConfigParser(cls)
    parser.add_argument("name", type=ObjectParser.safe_filename, required=True)
    parser.add_argument("sql", type=ObjectParser.non_empty_str, required=True)
    return parser


class CrossbenchTraceUriResolver(TraceUriResolver):
  PREFIX = "crossbench"

  def __init__(self, traces: Union[Iterable[Run], TraceProcessorProbeContext]):

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

  def resolve(self) -> List["TraceUriResolver.Result"]:
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
        type=TraceProcessorQueryConfig,
        is_list=True,
        default=tuple(),
        help="Name of query to be run (under probes/trace_processor/queries) "
        "or { name: str, sql: str } containing the name and SQL query to run")
    parser.add_argument(
        "trace_processor_bin",
        type=PathParser.local_binary_path,
        help="Path to the trace_processor binary")
    return parser

  def __init__(self,
               batch: bool,
               metrics: Iterable[str],
               queries: Iterable[TraceProcessorQueryConfig],
               trace_processor_bin: Optional[pth.LocalPath] = None):
    super().__init__()
    self._batch = batch
    self._metrics = tuple(metrics)
    ObjectParser.unique_sequence([query.name for query in queries],
                                 name="query names")
    self._queries = tuple(queries)
    self._trace_processor_bin: Optional[pth.LocalPath] = None
    if trace_processor_bin:
      self._trace_processor_bin = PathParser.local_binary_path(
          trace_processor_bin, "trace_processor")

  @property
  def batch(self) -> bool:
    return self._batch

  @property
  def metrics(self) -> Tuple[str, ...]:
    return self._metrics

  @property
  def queries(self) -> Tuple[TraceProcessorQueryConfig, ...]:
    return self._queries

  @property
  def has_work(self) -> bool:
    return len(self._queries) != 0 or len(self._metrics) != 0

  @property
  def needs_tp_run(self) -> bool:
    return (not self.batch) and self.has_work

  @property
  def needs_btp_run(self) -> bool:
    return self._batch and self.has_work

  @property
  def trace_processor_bin(self) -> Optional[pth.LocalPath]:
    return self._trace_processor_bin

  @property
  def tp_config(self) -> TraceProcessorConfig:
    extra_flags = [
        "--add-sql-module",
        _MODULES_DIR,
    ]

    return TraceProcessorConfig(
        bin_path=self.trace_processor_bin,
        resolver_registry=ResolverRegistry(
            resolvers=[CrossbenchTraceUriResolver, PathUriResolver]),
        load_timeout=10,
        extra_flags=extra_flags)

  def get_context_cls(self) -> Type[TraceProcessorProbeContext]:
    return TraceProcessorProbeContext

  def validate_env(self, env: HostEnvironment) -> None:
    super().validate_env(env)
    self._check_sql()

  def _check_sql(self) -> None:
    """
    Runs all metrics and queries on an empty trace. This will ensure that they
    are correctly defined in trace processor.
    """
    with TraceProcessor(trace="/dev/null", config=self.tp_config) as tp:
      for metric in self.metrics:
        tp.metric([metric])
      for query in self.queries:
        tp.query(query.sql)

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
    merged_metrics: Dict[str,
                         MetricsMerger] = collections.defaultdict(MetricsMerger)
    for run in runs:
      for file_path in run.results[self].json_list:
        with file_path.open() as json_file:
          merged_metrics[file_path.stem].add(json.load(json_file))

    return {
        metric_name: merged.to_json()
        for metric_name, merged in merged_metrics.items()
    }

  def merge_browsers(self, group: BrowsersRunGroup) -> ProbeResult:
    if self.needs_btp_run:
      return self._run_btp(group)

    return self._merge_browser_files(group)

  def _merge_browser_files(self, group: BrowsersRunGroup) -> LocalProbeResult:
    group_dir = group.get_local_probe_result_path(self)
    group_dir.mkdir()
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
        # TODO(375390958): figure out why files aren't fully written to
        # pyfakefs here.
        f.write("\n")
      json_files.append(json_file)
    return LocalProbeResult(csv=csv_files, json=json_files)

  def _run_btp(self, group: BrowsersRunGroup) -> LocalProbeResult:
    group_dir = group.get_local_probe_result_path(self)
    group_dir.mkdir()
    btp_config = BatchTraceProcessorConfig(tp_config=self.tp_config)

    with BatchTraceProcessor(
        traces=CrossbenchTraceUriResolver(group.runs),
        config=btp_config) as btp:

      def run_query(query: TraceProcessorQueryConfig):
        csv_file = group_dir / f"{query.name}.csv"
        btp.query_and_flatten(query.sql).to_csv(
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

  def get_default_result_path(self) -> pth.AnyPath:
    result_dir = super().get_default_result_path()
    self.host_platform.mkdir(result_dir)
    return result_dir

  def setup(self) -> None:
    pass

  def start(self) -> None:
    pass

  def stop(self) -> None:
    pass

  def teardown(self) -> ProbeResult:
    return self._merge_trace_files().merge(self._maybe_run_tp())

  def _merge_trace_files(self) -> LocalProbeResult:
    with self.run.actions("TRACE_PROCESSOR: Merging trace files", verbose=True):
      with zipfile.ZipFile(self.merged_trace_path, "w") as zip_file:
        for f in self.run.results.all_traces():
          zip_file.write(f, arcname=f.relative_to(self.run.out_dir))
    return LocalProbeResult(trace=(self.merged_trace_path,))

  def _maybe_run_tp(self):
    if not self.probe.needs_tp_run:
      return LocalProbeResult()

    with TraceProcessor(
        trace=CrossbenchTraceUriResolver(self),
        config=self.probe.tp_config) as tp:
      return self._run_queries(tp).merge(self._run_metrics(tp))

  def _run_queries(self, tp: TraceProcessor) -> LocalProbeResult:

    def run_query(query: TraceProcessorQueryConfig):
      csv_file = self.local_result_path / f"{query.name}.csv"
      tp.query(query.sql).as_pandas_dataframe().to_csv(
          path_or_buf=csv_file, index=False)
      return csv_file

    with self.run.actions("TRACE_PROCESSOR: Running queries", verbose=True):
      files = tuple(map(run_query, self.probe.queries))
      return LocalProbeResult(csv=files)

  def _run_metrics(self, tp: TraceProcessor) -> LocalProbeResult:

    def run_metric(metric: str):
      json_file = self.local_result_path / f"{pth.safe_filename(metric)}.json"
      proto = tp.metric([metric])
      with json_file.open("x") as f:
        f.write(MessageToJson(proto))
      return json_file

    with self.run.actions("TRACE_PROCESSOR: Running metrics", verbose=True):
      files = tuple(map(run_metric, self.probe.metrics))
      return LocalProbeResult(json=files)

  @property
  def merged_trace_path(self):
    return self.local_result_path / "merged_trace.zip"
