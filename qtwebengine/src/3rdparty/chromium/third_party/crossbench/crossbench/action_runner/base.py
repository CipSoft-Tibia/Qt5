# Copyright 2024 The Chromium Authors
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

from __future__ import annotations

from typing import TYPE_CHECKING, Iterable, List, Optional, Sequence

from crossbench import exception
from crossbench.action_runner.action_runner_listener import \
    ActionRunnerListener
from crossbench.action_runner.bond_base import BondActionRunner
from crossbench.action_runner.screenshot_annotation import ScreenshotAnnotation
from crossbench.benchmarks.loading.input_source import InputSource

if TYPE_CHECKING:
  from crossbench.action_runner.action import all as i_action
  from crossbench.benchmarks.loading.config.pages import ActionBlock
  from crossbench.benchmarks.loading.page.base import Page
  from crossbench.benchmarks.loading.page.combined import CombinedPage
  from crossbench.benchmarks.loading.page.interactive import InteractivePage
  from crossbench.benchmarks.loading.tab_controller import TabController
  from crossbench.runner.run import Run


class ActionNotImplementedError(NotImplementedError):

  def __init__(self,
               runner: ActionRunner,
               action: i_action.Action,
               msg_context: str = "") -> None:
    self.runner = runner
    self.action = action

    if msg_context:
      msg_context = ". Context: " + msg_context

    message = (f"{str(action.TYPE).capitalize()}-action "
               f"not implemented in {type(runner).__name__}{msg_context}")
    super().__init__(message)


class InputSourceNotImplementedError(ActionNotImplementedError):

  def __init__(self,
               runner: ActionRunner,
               action: i_action.Action,
               input_source: InputSource,
               msg_context: str = "") -> None:

    if msg_context:
      msg_context = ". Context: " + msg_context

    input_source_message = (f"Source: '{input_source}'"
                            f"not implemented{msg_context}")

    super().__init__(runner, action, input_source_message)

class ActionRunner:

  def __init__(self):
    self._listener = ActionRunnerListener()
    # TODO: Don't share state across runs
    self._info_stack: Optional[exception.TInfoStack] = None

    self._failure_screenshot_annotations: List[ScreenshotAnnotation] = []

  def set_listener(self, listener):
    self._listener = listener

  # info_stack is a unique identifier for the currently running or most recently
  # run action.
  @property
  def info_stack(self) -> exception.TInfoStack:
    if not self._info_stack:
      raise RuntimeError("info_stack can not be called before run_blocks")
    return self._info_stack

  @property
  def bond(self) -> BondActionRunner:
    return BondActionRunner()

  def teardown(self, run: Run):
    del run
    pass

  def run_blocks(self, run: Run, page: InteractivePage,
                 blocks: Iterable[ActionBlock]) -> None:
    for block in blocks:
      block.run_with(self, run, page)

  def run_block(self, run, block: ActionBlock) -> None:
    block_index = block.index
    # TODO: Instead maybe just pass context down.
    # Or pass unique path to every action __init__
    with exception.annotate(f"Running block {block_index}: {block.label}"):
      for action_index, action in enumerate(block, start=1):
        self._info_stack = (f"block_{block_index}", f"action_{action_index}")
        self._failure_screenshot_annotations = []
        action.run_with(run, self)

  def wait(self, run: Run, action: i_action.WaitAction) -> None:
    with run.actions("WaitAction", measure=False) as actions:
      actions.wait(action.duration)

  def js(self, run: Run, action: i_action.JsAction) -> None:
    with run.actions("JS", measure=False) as actions:
      actions.js(action.script, action.timeout)

  def click(self, run: Run, action: i_action.ClickAction) -> None:
    input_source = action.input_source
    if input_source is InputSource.JS:
      self.click_js(run, action)
    elif input_source is InputSource.TOUCH:
      self.click_touch(run, action)
    elif input_source is InputSource.MOUSE:
      self.click_mouse(run, action)
    else:
      raise RuntimeError(f"Unsupported input source: '{input_source}'")

  def scroll(self, run: Run, action: i_action.ScrollAction) -> None:
    input_source = action.input_source
    if input_source is InputSource.JS:
      self.scroll_js(run, action)
    elif input_source is InputSource.TOUCH:
      self.scroll_touch(run, action)
    elif input_source is InputSource.MOUSE:
      self.scroll_mouse(run, action)
    else:
      raise RuntimeError(f"Unsupported input source: '{input_source}'")

  def get(self, run: Run, action: i_action.GetAction) -> None:
    raise ActionNotImplementedError(self, action)

  def text_input(self, run: Run, action: i_action.TextInputAction) -> None:
    input_source = action.input_source
    if input_source is InputSource.JS:
      self.text_input_js(run, action)
    elif input_source is InputSource.KEYBOARD:
      self.text_input_keyboard(run, action)
    else:
      raise RuntimeError(f"Unsupported input source: '{input_source}'")

  def click_js(self, run: Run, action: i_action.ClickAction) -> None:
    raise InputSourceNotImplementedError(self, action, action.input_source)

  def click_touch(self, run: Run, action: i_action.ClickAction) -> None:
    raise InputSourceNotImplementedError(self, action, action.input_source)

  def click_mouse(self, run: Run, action: i_action.ClickAction) -> None:
    raise InputSourceNotImplementedError(self, action, action.input_source)

  def scroll_js(self, run: Run, action: i_action.ScrollAction) -> None:
    raise InputSourceNotImplementedError(self, action, action.input_source)

  def scroll_touch(self, run: Run, action: i_action.ScrollAction) -> None:
    raise InputSourceNotImplementedError(self, action, action.input_source)

  def scroll_mouse(self, run: Run, action: i_action.ScrollAction) -> None:
    raise InputSourceNotImplementedError(self, action, action.input_source)

  def text_input_js(self, run: Run, action: i_action.TextInputAction) -> None:
    raise InputSourceNotImplementedError(self, action, action.input_source)

  def text_input_keyboard(self, run: Run,
                          action: i_action.TextInputAction) -> None:
    raise InputSourceNotImplementedError(self, action, action.input_source)

  def swipe(self, run: Run, action: i_action.SwipeAction) -> None:
    raise ActionNotImplementedError(self, action)

  def wait_for_element(self, run: Run,
                       action: i_action.WaitForElementAction) -> None:
    raise ActionNotImplementedError(self, action)

  def wait_for_ready_state(self, run: Run,
                           action: i_action.WaitForReadyStateAction) -> None:
    raise ActionNotImplementedError(self, action)

  def inject_new_document_script(
      self, run: Run, action: i_action.InjectNewDocumentScriptAction) -> None:
    raise ActionNotImplementedError(self, action)

  def screenshot_impl(
      self,
      run: Run,
      suffix: str,
      annotations: Optional[Sequence[ScreenshotAnnotation]] = None) -> None:
    del run, suffix, annotations
    raise NotImplementedError("screenshot_impl not implemented")

  def add_failure_screenshot_annotation(
      self, annotation: ScreenshotAnnotation) -> None:
    self._failure_screenshot_annotations.append(annotation)

  def failure_screenshot(self, run: Run, suffix: str) -> None:
    self.screenshot_impl(run, suffix, self._failure_screenshot_annotations)

  def screenshot(self, run: Run, action: i_action.ScreenshotAction) -> None:
    del action
    with run.actions("Screenshot", measure=False):
      self.screenshot_impl(run, "screenshot")

  def dump_html_impl(self, run: Run, suffix: str) -> None:
    del run, suffix
    raise NotImplementedError("dump_html_impl not implemented")

  def dump_html(self, run: Run, action: i_action.DumpHtmlAction) -> None:
    del action
    with run.actions("Dump HTML", measure=False):
      self.dump_html_impl(run, "dump")

  def _maybe_navigate_to_about_blank(self, run: Run, page: Page) -> None:
    if duration := page.about_blank_duration:
      run.browser.show_url("about:blank")
      run.runner.wait(duration)

  def run_page_multiple_tabs(self, run: Run, tabs: TabController,
                             pages: Iterable[Page]):
    # TODO: refactor possible logics to TabController.
    browser = run.browser
    for _ in tabs:
      try:
        for i, page in enumerate(pages):
          # Create a new tab for the multiple_tab case.
          if i > 0:
            browser.switch_to_new_tab()
            self._listener.handle_new_tab(run)
          page.run_with(run, self, False)
          self._listener.handle_page_run(run)
        browser.switch_to_new_tab()
        self._listener.handle_new_tab(run)
      except Exception as e:
        self._listener.handle_error(run, e)
        raise

  def run_combined_page(self, run: Run, page: CombinedPage,
                        multiple_tabs: bool):
    if multiple_tabs:
      self.run_page_multiple_tabs(run, page.tabs, page.pages)
    else:
      for sub_page in page.pages:
        sub_page.run_with(run, self, False)

  def run_interactive_page_once(self, run: Run, page: InteractivePage):
    try:
      self.run_blocks(run, page, page.blocks)
      self._maybe_navigate_to_about_blank(run, page)
    except Exception:
      page.create_failure_artifacts(run)
      raise

  def run_interactive_page(self, run: Run, page: InteractivePage,
                           multiple_tabs: bool):
    if multiple_tabs:
      self.run_page_multiple_tabs(run, page.tabs, [page])
    else:
      self.run_interactive_page_once(run, page)

  def run_setup(self, run: Run, page: InteractivePage, setup: ActionBlock):
    try:
      with exception.annotate("setup"):
        setup.run_with(self, run, page)
    except Exception:
      page.create_failure_artifacts(run, "setup-failure")
      raise

  def run_login(self, run: Run, page: InteractivePage, login: ActionBlock):
    try:
      with exception.annotate("login"):
        with run.browser.network.traffic_shaper.pause():
          login.run_with(self, run, page)
    except Exception:
      page.create_failure_artifacts(run, "login-failure")
      raise

  def switch_tab(self, run: Run, action: i_action.SwitchTabAction):
    raise ActionNotImplementedError(self, action)
