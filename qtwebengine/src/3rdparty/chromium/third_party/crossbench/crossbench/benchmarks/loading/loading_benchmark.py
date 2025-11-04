# Copyright 2023 The Chromium Authors
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

from __future__ import annotations

import argparse
import datetime as dt
import logging
from typing import (TYPE_CHECKING, Any, Dict, List, Optional, Sequence, Tuple,
                    Type)

from crossbench import cli_helper
from crossbench.benchmarks.base import StoryFilter, SubStoryBenchmark
from crossbench.benchmarks.loading.action_runner.base import ActionRunner
from crossbench.benchmarks.loading.action_runner.basic_action_runner import \
    BasicActionRunner
from crossbench.benchmarks.loading.action_runner.config import \
    ActionRunnerConfig
from crossbench.benchmarks.loading.config.pages import (
    DevToolsRecorderPagesConfig, ListPagesConfig, PageConfig, PagesConfig)
from crossbench.benchmarks.loading.page import (DEFAULT_DURATION, PAGE_LIST,
                                                PAGE_LIST_SMALL, PAGES,
                                                CombinedPage, InteractivePage,
                                                LivePage, Page)
from crossbench.benchmarks.loading.playback_controller import \
    PlaybackController
from crossbench.benchmarks.loading.tab_controller import TabController

if TYPE_CHECKING:
  from crossbench.benchmarks.loading.action_runner.base import ActionRunner
  from crossbench.cli.parser import CrossBenchArgumentParser
  from crossbench.stories.story import Story


class LoadingPageFilter(StoryFilter[Page]):
  """
  Filter / create loading stories

  Syntax:
    "name"            Include LivePage with the given name from predefined list.
    "name", 10        Include predefined page with given 10s timeout.
    "http://..."      Include custom page at the given URL with a default
                      timeout of 15 seconds.
    "http://...", 12  Include custom page at the given URL with a 12s timeout

  These patterns can be combined:
    ["http://foo.com", 5, "http://bar.co.jp", "amazon"]
  """
  stories: Sequence[Page]

  @classmethod
  def add_cli_parser(
      cls, parser: argparse.ArgumentParser) -> argparse.ArgumentParser:
    parser = super().add_cli_parser(parser)
    cls.add_page_config_parser(parser)
    tab_group = parser.add_mutually_exclusive_group()
    tab_group.add_argument(
        "--single-tab",
        dest="tabs",
        const=TabController.single(),
        default=TabController.default(),
        action="store_const",
        help="Open given urls in a single tab.")
    tab_group.add_argument(
        "--multiple-tab",
        dest="tabs",
        nargs='?',
        type=TabController.parse,
        const=TabController.multiple(),
        help="Open given urls in separate tabs (optional value for number of tabs for each url)."
    )
    tab_group.add_argument(
        "--infinite-tab",
        dest="tabs",
        const=TabController.forever(),
        action="store_const",
        help="Open given urls in seperate tabs infinitely.")

    playback_group = parser.add_mutually_exclusive_group()
    playback_group.add_argument(
        "--playback",
        "--cycle",
        type=PlaybackController.parse,
        default=PlaybackController.default(),
        help="Set limit on looping through/repeating the selected stories. "
        "Default is once."
        "Valid values are: 'once', 'forever', number, time. "
        "Cycle 10 times: '--playback=10x'. "
        "Repeat for 1.5 hours: '--playback=1.5h'.")
    playback_group.add_argument(
        "--forever",
        dest="playback",
        const=PlaybackController.forever(),
        action="store_const",
        help="Equivalent to --playback=infinity")

    parser.add_argument(
        "--about-blank-duration",
        "--about-blank",
        type=cli_helper.Duration.parse_zero,
        default=dt.timedelta(),
        help="If non-zero, navigate to about:blank after every page.")

    return parser

  @classmethod
  def add_page_config_parser(cls, parser):
    page_config_group = parser.add_mutually_exclusive_group()
    # TODO: move --stories into mutually exclusive group as well
    page_config_group.add_argument(
        "--urls",
        "--url",
        dest="urls",
        help="List of urls and durations to load: url,seconds,...")
    page_config_group.add_argument(
        "--page-config",
        "--pages-config",
        dest="pages_config",
        type=PagesConfig.parse,
        help="Stories we want to perform in the benchmark run following a"
        "specified scenario. For a reference on how to build scenarios and"
        "possible actions check config/doc/pages.config.hjson")
    page_config_group.add_argument(
        "--url-file",
        "--urls-file",
        dest="pages_config",
        type=ListPagesConfig.parse,
        help=("List of urls and durations in a line-by-line file. "
              "Each line has the same format as --url for a single Page."))
    page_config_group.add_argument(
        "--devtools-recorder",
        dest="pages_config",
        type=DevToolsRecorderPagesConfig.parse,
        help=("Run a single story from a serialized DevTools recorder session. "
              "See https://developer.chrome.com/docs/devtools/recorder/ "
              "for more details."))

  @classmethod
  def kwargs_from_cli(cls, args: argparse.Namespace) -> Dict[str, Any]:
    kwargs = super().kwargs_from_cli(args)
    kwargs["separate"] = args.separate
    kwargs["args"] = args
    return kwargs

  def __init__(self,
               story_cls: Type[Page],
               patterns: Sequence[str],
               args: argparse.Namespace,
               separate: bool = True) -> None:
    self._args: argparse.Namespace = args
    super().__init__(story_cls, patterns, separate)

  def process_all(self, patterns: Sequence[str]) -> None:
    name_or_url_list = patterns
    if len(name_or_url_list) == 1:
      if name_or_url_list[0] == "all":
        self.stories = self.all_stories()
        return
      if name_or_url_list[0] == "default":
        self.stories = self.default_stories()
        return
    # Let the PageConfig handle the arg splitting again:
    config = PagesConfig.parse(",".join(patterns))
    self.stories = self.stories_from_config(self._args, config)

  @classmethod
  def all_stories(cls) -> Tuple[Page, ...]:
    return PAGE_LIST

  @classmethod
  def default_stories(cls) -> Tuple[Page, ...]:
    return PAGE_LIST_SMALL

  @classmethod
  def stories_from_config(cls, args: argparse.Namespace,
                          config: PagesConfig) -> Sequence[Page]:
    labels = set(page_config.label for page_config in config.pages)
    use_labels = len(labels) == len(config.pages)

    stories: List[Page] = []
    for page_config in config.pages:
      stories.append(cls._story_from_config(args, page_config, use_labels))

    if use_labels:
      # Double check that the urls are unique
      urls = set(page_config.first_url for page_config in config.pages)
      if len(urls) != len(config.pages):
        raise argparse.ArgumentTypeError(
            "Got non-unique story labels and urls.")
    return stories

  @classmethod
  def _story_from_config(cls, args: argparse.Namespace, config: PageConfig,
                         use_labels: bool) -> Page:
    playback: PlaybackController = args.playback
    tabs: TabController = args.tabs
    if config.playback:
      # TODO: support custom config playback
      playback = config.playback
    duration: dt.timedelta = config.duration
    if config.label in PAGES:
      page = PAGES[config.label]
      duration = duration or page.duration
      return LivePage(page.name, page.url, duration, playback, tabs,
                      args.about_blank_duration)

    label: str = config.any_label if use_labels else config.first_url
    duration = duration or DEFAULT_DURATION

    if not config.blocks:
      return LivePage(label, config.first_url, duration, playback, tabs,
                      args.about_blank_duration)
    return InteractivePage(label, config.blocks, config.login, playback, tabs,
                           args.about_blank_duration)

  def create_stories(self, separate: bool) -> Sequence[Page]:
    logging.info("SELECTED STORIES: %s", str(list(map(str, self.stories))))
    if not separate and len(self.stories) > 1:
      combined_name = "_".join(page.name for page in self.stories)
      self.stories = (CombinedPage(self.stories, combined_name,
                                   self._args.playback, self._args.tabs),)
    return self.stories


class PageLoadBenchmark(SubStoryBenchmark):
  """
  Benchmark runner for loading pages.

  Use --urls/--stories to either choose from an existing set of pages, or direct
  URLs. After each page you can also specify a custom wait/load duration in
  seconds. Multiple URLs/page names can be provided as a comma-separated list.

  Use --separate to load each page individually.

  Example:
    --urls=amazon
    --urls=http://cnn.com,10s
    --urls=http://twitter.com,5s,http://cnn.com,10s
  """
  NAME = "loading"
  DEFAULT_STORY_CLS = Page
  STORY_FILTER_CLS = LoadingPageFilter

  @classmethod
  def add_cli_parser(
      cls, subparsers: argparse.ArgumentParser, aliases: Sequence[str] = ()
  ) -> CrossBenchArgumentParser:
    parser = super().add_cli_parser(subparsers, aliases)
    cls.STORY_FILTER_CLS.add_cli_parser(parser)

    parser.add_argument(
        "--action-runner",
        type=ActionRunnerConfig.parse,
        help="Set the action runner for interactive pages.")
    return parser

  @classmethod
  def requires_separate(cls, args: argparse.Namespace):
    return args.separate

  @classmethod
  def stories_from_cli_args(cls, args: argparse.Namespace) -> Sequence[Story]:
    has_default_stories: bool = args.stories and args.stories == "default"
    if config := cls.get_pages_config(args):
      # TODO: make stories and page_config mutually exclusive.
      if not has_default_stories:
        raise argparse.ArgumentTypeError(
            f"Cannot specify --stories={repr(args.stories)} "
            "with any other page config option.")
      pages = LoadingPageFilter.stories_from_config(args, config)
      if cls.requires_separate(args):
        assert len(config.logins) == 0
        return pages
      if len(pages) == 1 and len(config.logins) == 0:
        return pages
      return (CombinedPage(
          pages,
          "Page Scenarios - Combined",
          args.playback,
          args.tabs,
          logins=config.logins),)

    if args.urls:
      # TODO: make urls and stories mutually exclusive.
      if not has_default_stories:
        raise argparse.ArgumentTypeError(
            "Cannot specify --urls and --stories at the same time.")
      args.stories = args.urls

    # Fall back to story filter class.
    return super().stories_from_cli_args(args)

  @classmethod
  def get_pages_config(cls, args: argparse.Namespace) -> Optional[PagesConfig]:
    if global_config := args.config:
      # TODO: migrate --config to an already parsed hjson/json dict
      config_file = global_config
      config_data = cli_helper.parse_hjson_file(config_file)
      if pages_config_dict := config_data.get("pages"):
        if args.pages_config:
          raise argparse.ArgumentTypeError(
              "Conflicting arguments: "
              "either specify a --config file without a 'pages' property "
              "or remove the --page-config argument.")
        # TODO: PagesConfig.parse_dict should be able to parse the inner dict.
        return PagesConfig.parse_dict({"pages": pages_config_dict})
    return args.pages_config

  @classmethod
  def aliases(cls) -> Tuple[str, ...]:
    return ("load", "ld")

  @classmethod
  def kwargs_from_cli(cls, args: argparse.Namespace) -> Dict[str, Any]:
    kwargs = super().kwargs_from_cli(args)
    kwargs["action_runner"] = args.action_runner
    return kwargs

  def __init__(self,
               stories: Sequence[Page],
               action_runner: Optional[ActionRunner] = None) -> None:
    self._action_runner = action_runner or BasicActionRunner()
    for story in stories:
      assert isinstance(story, Page)
    super().__init__(stories)

  @property
  def action_runner(self) -> ActionRunner:
    return self._action_runner
