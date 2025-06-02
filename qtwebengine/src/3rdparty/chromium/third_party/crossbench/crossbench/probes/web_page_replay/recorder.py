# Copyright 2023 The Chromium Authors
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

from __future__ import annotations

import argparse
import pathlib
import shutil
from typing import TYPE_CHECKING, Iterable, List, Optional

from frozendict import frozendict

from crossbench import cli_helper, helper, plt
from crossbench.browsers.chromium.chromium import Chromium
from crossbench.network.web_page_replay import WprRecorder
from crossbench.probes import helper as probe_helper
from crossbench.probes.probe import Probe, ProbeConfigParser, ProbeContext
from crossbench.probes.results import EmptyProbeResult, ProbeResult
from crossbench.runner.groups import (BrowsersRunGroup, RepetitionsRunGroup,
                                      RunGroup, StoriesRunGroup)

if TYPE_CHECKING:
  from crossbench.browsers.browser import Browser
  from crossbench.runner.run import Run



class WebPageReplayProbe(Probe):
  """
  Probe to collect browser requests to wpr.go archive which then can be
  replayed using a local proxy server.

  Chrome telemetry's wpr.go:
  https://chromium.googlesource.com/catapult/+/HEAD/web_page_replay_go/README.md
  """

  NAME = "wpr"

  @classmethod
  def config_parser(cls) -> ProbeConfigParser:
    parser = super().config_parser()
    parser.add_argument("http_port", type=int, default=8080, required=False)
    parser.add_argument("https_port", type=int, default=8081, required=False)
    parser.add_argument(
        "wpr_go_bin", type=cli_helper.parse_binary_path, required=False)
    parser.add_argument(
        "key_file", type=cli_helper.parse_existing_file_path, required=False)
    parser.add_argument(
        "cert_file", type=cli_helper.parse_existing_file_path, required=False)
    parser.add_argument(
        "inject_scripts",
        is_list=True,
        type=cli_helper.parse_existing_file_path,
        required=False)
    parser.add_argument(
        "use_test_root_certificate", type=bool, default=False, required=False)
    return parser

  def __init__(self,
               http_port: int = 0,
               https_port: int = 0,
               wpr_go_bin: Optional[pathlib.Path] = None,
               inject_scripts: Optional[Iterable[pathlib.Path]] = None,
               key_file: Optional[pathlib.Path] = None,
               cert_file: Optional[pathlib.Path] = None,
               use_test_root_certificate: bool = False):
    super().__init__()
    if not wpr_go_bin:
      wpr_go_bin = WprGoToolFinder(plt.PLATFORM).path
    if not wpr_go_bin:
      raise argparse.ArgumentTypeError(f"'{wpr_go_bin}' does not exist.")
    self._wpr_go_bin = cli_helper.parse_non_empty_file_path(wpr_go_bin)

    self._recorder_kwargs = frozendict(
        bin_path=wpr_go_bin,
        http_port=http_port,
        https_port=https_port,
        inject_scripts=inject_scripts,
        key_file=key_file,
        cert_file=cert_file,
    )

    self._https_port = https_port
    self._http_port = http_port
    self._use_test_root_certificate = use_test_root_certificate

  @property
  def https_port(self) -> int:
    return self._https_port

  @property
  def http_port(self) -> int:
    return self._http_port

  @property
  def recorder_kwargs(self) -> frozendict:
    return self._recorder_kwargs

  @property
  def use_test_root_certificate(self) -> bool:
    return self._use_test_root_certificate

  @property
  def result_path_name(self) -> str:
    return "archive.wprgo"

  def is_compatible(self, browser: Browser) -> bool:
    return isinstance(browser, Chromium) and browser.platform.is_local

  def get_context(self, run: Run) -> WprRecorderProbeContext:
    return WprRecorderProbeContext(self, run)

  def merge_repetitions(self, group: RepetitionsRunGroup) -> ProbeResult:
    results = [run.results[self].file for run in group.runs]
    return self.merge_group(results, group)

  def merge_stories(self, group: StoriesRunGroup) -> ProbeResult:
    results = [
        subgroup.results[self].file for subgroup in group.repetitions_groups
    ]
    return self.merge_group(results, group)

  def merge_browsers(self, group: BrowsersRunGroup) -> ProbeResult:
    results = [subgroup.results[self].file for subgroup in group.story_groups]
    return self.merge_group(results, group)

  def merge_group(self, results: List[pathlib.Path],
                  group: RunGroup) -> ProbeResult:
    result_file = group.get_local_probe_result_path(self)
    if not results:
      return EmptyProbeResult()
    first_wprgo = results.pop(0)
    # TODO migrate to platform
    shutil.copy(first_wprgo, result_file)
    for repetition_file in results:
      self.httparchive_merge(repetition_file, result_file)
    return ProbeResult(file=[result_file])

  def httparchive_merge(self, input_archive: pathlib.Path,
                        output_archive: pathlib.Path) -> None:
    cmd = [
        "go",
        "run",
        self._wpr_go_bin.parent / "httparchive.go",
        "merge",
        output_archive,
        input_archive,
        output_archive,
    ]
    with helper.ChangeCWD(self._wpr_go_bin.parent):
      self.runner_platform.sh(*cmd)


class WprRecorderProbeContext(ProbeContext[WebPageReplayProbe]):

  def __init__(self, probe: WebPageReplayProbe, run: Run) -> None:
    super().__init__(probe, run)
    self._wprgo_log: pathlib.Path = self.result_path.with_name("wpr_record.log")
    self._host: str = "127.0.0.1"
    kwargs = dict(self.probe.recorder_kwargs)
    kwargs.update({
        "platform": run.runner_platform,
        "log_path": self._wprgo_log,
        "result_path": self.result_path,
    })
    self._recorder = WprRecorder(**kwargs)

  def setup(self) -> None:
    self._recorder.start()
    self._setup_extra_flags()

  def _setup_extra_flags(self) -> None:
    if not self.probe.use_test_root_certificate:
      cert_hash_file = self._recorder.cert_file.parent / "wpr_public_hash.txt"
      if not cert_hash_file.is_file():
        raise ValueError(
            f"Could not read public key hash file: {cert_hash_file}")
      cert_skip_list = ",".join(cert_hash_file.read_text().strip().splitlines())
      self.session.extra_flags[
          "--ignore-certificate-errors-spki-list"] = cert_skip_list
    # TODO: support ts_proxy traffic shaping
    # session.extra_flags[
    #     "--proxy-server"] = "socks://{self._ts_proxy_host}:{self._ts_proxy_port}"
    # session.extra_flags["--proxy-bypass-list"] = "<-loopback>"
    self.session.extra_flags["--host-resolver-rules"] = (
        f"MAP *:80 {self._host}:{self._recorder.http_port},"
        f"MAP *:443 {self._host}:{self._recorder.https_port},"
        "EXCLUDE localhost")
    # TODO: add replay support, see:
    # third_party/catapult/telemetry/telemetry/internal/backends/chrome/chrome_startup_args.py

  def start(self) -> None:
    pass

  def stop(self) -> None:
    pass

  def tear_down(self) -> ProbeResult:
    self._recorder.stop()
    return self.browser_result(file=(self.result_path,))


class WprGoToolFinder:
  _WPR_GO = pathlib.Path("third_party/catapult/web_page_replay_go/src/wpr.go")

  def __init__(self, platform: plt.Platform) -> None:
    self.platform = platform
    self.path: Optional[pathlib.Path] = None
    if maybe_chrome := probe_helper.ChromiumCheckoutFinder(platform).path:
      candidate = (maybe_chrome / self._WPR_GO)
      if self.platform.is_file(candidate):
        self.path = candidate
