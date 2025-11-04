# Copyright 2023 The Chromium Authors
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

from __future__ import annotations

import abc
import datetime as dt
import logging
from typing import TYPE_CHECKING, Iterable, Optional, Tuple, cast

from crossbench.benchmarks.loading.action import ActionType, GetAction
from crossbench.benchmarks.loading.action_runner.base import \
    ActionNotImplementedError
from crossbench.benchmarks.loading.playback_controller import \
    PlaybackController
from crossbench.benchmarks.loading.tab_controller import TabController
from crossbench.browsers.secrets import SecretType
from crossbench.stories.story import Story

if TYPE_CHECKING:
  from crossbench.benchmarks.loading.action_runner.base import ActionRunner
  from crossbench.benchmarks.loading.config.login.custom import LoginBlock
  from crossbench.benchmarks.loading.config.pages import ActionBlock
  from crossbench.benchmarks.loading.loading_benchmark import PageLoadBenchmark
  from crossbench.runner.run import Run
  from crossbench.types import JsonDict

DEFAULT_DURATION_SECONDS = 15
DEFAULT_DURATION = dt.timedelta(seconds=DEFAULT_DURATION_SECONDS)


class Page(Story, metaclass=abc.ABCMeta):

  @classmethod
  def all_story_names(cls) -> Tuple[str, ...]:
    return tuple(page.name for page in PAGE_LIST)

  def __init__(self,
               name: str,
               duration: dt.timedelta = DEFAULT_DURATION,
               playback: PlaybackController = PlaybackController.default(),
               tabs: TabController = TabController.default(),
               about_blank_duration: dt.timedelta = dt.timedelta()):
    self._playback: PlaybackController = playback
    self._tabs: TabController = tabs
    self._about_blank_duration = about_blank_duration
    super().__init__(name, duration)

  @property
  def about_blank_duration(self) -> dt.timedelta:
    return self._about_blank_duration

  def set_parent(self, parent: Page) -> None:
    # TODO: support nested playback controllers.
    self._playback = PlaybackController.default()
    self._tabs = TabController.default()
    del parent

  @abc.abstractmethod
  def run_with(self, run: Run, action_runner: ActionRunner,
               multiple_tabs: bool) -> None:
    pass

  @property
  @abc.abstractmethod
  def first_url(self) -> str:
    pass

  @property
  def tabs(self) -> TabController:
    return self._tabs


def get_action_runner(run: Run) -> ActionRunner:
  # TODO: make sure we have a single instance per Run
  benchmark = cast("PageLoadBenchmark", run.benchmark)
  return benchmark.action_runner


class LivePage(Page):
  url: str

  def __init__(
      self,
      name: str,
      url: str,
      duration: dt.timedelta = DEFAULT_DURATION,
      playback: PlaybackController = PlaybackController.default(),
      tabs: TabController = TabController.default(),
      about_blank_duration: dt.timedelta = dt.timedelta()
  ) -> None:
    super().__init__(name, duration, playback, tabs, about_blank_duration)
    assert url, "Invalid page url"
    self.url: str = url

  def set_duration(self, duration: dt.timedelta) -> None:
    self._duration = duration

  def details_json(self) -> JsonDict:
    result = super().details_json()
    result["url"] = str(self.url)
    return result

  def run(self, run: Run) -> None:
    action_runner = get_action_runner(run)
    multiple_tabs = self.tabs.multiple_tabs
    for _ in self._playback:
      action_runner.run_page(run, self, multiple_tabs)

  def run_with(self, run: Run, action_runner: ActionRunner,
               multiple_tabs: bool) -> None:
    action_runner.run_page(run, self, multiple_tabs)

  @property
  def first_url(self) -> str:
    return self.url

  def __str__(self) -> str:
    return f"Page(name={self.name}, url={self.url})"


class CombinedPage(Page):

  def __init__(self,
               pages: Iterable[Page],
               name: str = "combined",
               playback: PlaybackController = PlaybackController.default(),
               tabs: TabController = TabController.default(),
               about_blank_duration: dt.timedelta = dt.timedelta(),
               logins: Optional[Iterable[SecretType]] = None):
    self._pages = tuple(pages)
    assert self._pages, "No sub-pages provided for CombinedPage"
    assert len(self._pages) > 1, "Combined Page needs more than one page"
    self._tabs = tabs
    self._logins = logins or []

    duration = dt.timedelta()
    for page in self._pages:
      page.set_parent(self)
      duration += page.duration
    super().__init__(name, duration, playback, tabs, about_blank_duration)
    self.url = None

  @property
  def tabs(self) -> TabController:
    return self._tabs

  @property
  def pages(self) -> Iterable[Page]:
    return self._pages

  @property
  def first_url(self) -> str:
    return self._pages[0].first_url

  def details_json(self) -> JsonDict:
    result = super().details_json()
    result["pages"] = list(page.details_json() for page in self._pages)
    return result

  def setup(self, run: Run) -> None:
    run.do_logins(self._logins)

  def run(self, run: Run) -> None:
    action_runner = get_action_runner(run)
    multiple_tabs = self.tabs.multiple_tabs
    for _ in self._playback:
      action_runner.run_combined_page(run, self, multiple_tabs)

  def run_with(self, run: Run, action_runner: ActionRunner,
               multiple_tabs: bool) -> None:
    action_runner.run_combined_page(run, self, multiple_tabs)

  def __str__(self) -> str:
    combined_name = ",".join(page.name for page in self._pages)
    return f"CombinedPage({combined_name})"


class InteractivePage(Page):

  def __init__(self,
               name: str,
               blocks: Tuple[ActionBlock, ...],
               login: Optional[LoginBlock] = None,
               playback: PlaybackController = PlaybackController.default(),
               tabs: TabController = TabController.default(),
               about_blank_duration: dt.timedelta = dt.timedelta()):
    assert name, "missing name"
    self._name: str = name
    assert isinstance(blocks, tuple)
    self._blocks: Tuple[ActionBlock, ...] = blocks
    assert self._blocks, "Must have at least 1 valid action"
    assert not any(block.is_login for block in blocks), (
        "No login blocks allowed as normal action block")
    self._login = login
    duration = self._get_duration()
    super().__init__(self._name, duration, playback, tabs, about_blank_duration)

  @property
  def blocks(self) -> Tuple[ActionBlock, ...]:
    return self._blocks

  @property
  def login(self) -> Optional[ActionBlock]:
    return self._login

  @property
  def first_url(self) -> str:
    for block in self.blocks:
      for action in block:
        if action.TYPE == ActionType.GET:
          return cast(GetAction, action).url
    raise RuntimeError("No GET action with an URL found.")

  def failure_screenshot(self, run: Run, message: str = "failure") -> None:
    action_runner = get_action_runner(run)
    try:
      action_runner.screenshot_impl(run, message)
    except ActionNotImplementedError:
      logging.debug("Skipping failure screenshot, action not implemented")
    except Exception as e:  # pylint: disable=broad-except
      logging.error("Failed to take a failure screenshot: %s", str(e))

  def setup(self, run: Run) -> None:
    if login := self.login:
      action_runner = get_action_runner(run)
      action_runner.run_login(run, self, login)

  def run(self, run: Run) -> None:
    action_runner = get_action_runner(run)
    multiple_tabs = self.tabs.multiple_tabs
    for _ in self._playback:
      action_runner.run_interactive_page(run, self, multiple_tabs)

  def run_with(self, run: Run, action_runner: ActionRunner,
               multiple_tabs: bool) -> None:
    action_runner.run_interactive_page(run, self, multiple_tabs)

  def details_json(self) -> JsonDict:
    result = super().details_json()
    result["actions"] = list(block.to_json() for block in self._blocks)
    return result

  def _get_duration(self) -> dt.timedelta:
    duration = dt.timedelta()
    for block in self._blocks:
      duration += block.duration
    return duration


PAGE_LIST = (LivePage("blank", "about:blank", dt.timedelta(seconds=1)),
             LivePage("amazon", "https://www.amazon.de/s?k=heizkissen",
                      dt.timedelta(seconds=5)),
             LivePage("bing",
                      "https://www.bing.com/images/search?q=not+a+squirrel",
                      dt.timedelta(seconds=5)),
             LivePage("caf", "http://www.caf.fr", dt.timedelta(seconds=6)),
             LivePage("cnn", "https://cnn.com/", dt.timedelta(seconds=7)),
             LivePage("ecma262",
                      "https://tc39.es/ecma262/#sec-numbers-and-dates",
                      dt.timedelta(seconds=10)),
             LivePage("expedia", "https://www.expedia.com/",
                      dt.timedelta(seconds=7)),
             LivePage("facebook", "https://facebook.com/shakira",
                      dt.timedelta(seconds=8)),
             LivePage("maps", "https://goo.gl/maps/TEZde4y4Hc6r2oNN8",
                      dt.timedelta(seconds=10)),
             LivePage("microsoft", "https://microsoft.com/",
                      dt.timedelta(seconds=6)),
             LivePage("provincial", "http://www.provincial.com",
                      dt.timedelta(seconds=6)),
             LivePage("sueddeutsche", "https://www.sueddeutsche.de/wirtschaft",
                      dt.timedelta(seconds=8)),
             LivePage("theverge", "https://www.theverge.com/",
                      dt.timedelta(seconds=10)),
             LivePage("timesofindia", "https://timesofindia.indiatimes.com/",
                      dt.timedelta(seconds=8)),
             LivePage("twitter", "https://twitter.com/wernertwertzog?lang=en",
                      dt.timedelta(seconds=6)))

PAGES = {page.name: page for page in PAGE_LIST}
PAGE_LIST_SMALL = (PAGES["facebook"], PAGES["maps"], PAGES["timesofindia"],
                   PAGES["cnn"])
