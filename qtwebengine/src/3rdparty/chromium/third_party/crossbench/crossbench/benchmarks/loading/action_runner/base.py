# Copyright 2024 The Chromium Authors
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

from __future__ import annotations

import abc
from typing import TYPE_CHECKING, Iterable, Optional

from crossbench import exception
from crossbench.benchmarks.loading.input_source import InputSource

if TYPE_CHECKING:
  from crossbench.benchmarks.loading import action as i_action
  from crossbench.benchmarks.loading.config.pages import ActionBlock
  from crossbench.benchmarks.loading.page import (CombinedPage, InteractivePage,
                                                  LivePage, Page)
  from crossbench.benchmarks.loading.tab_controller import TabController
  from crossbench.path import LocalPath
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
  # TODO: Don't share state across runs
  _info_stack: Optional[exception.TInfoStack]

  # info_stack is a unique identifier for the currently running or most recently
  # run action.
  @property
  def info_stack(self) -> exception.TInfoStack:
    if not self._info_stack:
      raise RuntimeError("info_stack can not be called before run_blocks")
    return self._info_stack

  def run_blocks(self, run: Run, blocks: Iterable[ActionBlock]) -> None:
    for block in blocks:
      self.run_block(run, block)

  def run_block(self, run, block: ActionBlock) -> None:
    block_index = block.index
    # TODO: Instead maybe just pass context down.
    # Or pass unique path to every action __init__
    with exception.annotate(f"Running block {block_index}: {block.label}"):
      for action_index, action in enumerate(block, start=1):
        self._info_stack = (f"block_{block_index}", f"action_{action_index}")
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

  def inject_new_document_script(
      self, run: Run, action: i_action.InjectNewDocumentScriptAction) -> None:
    raise ActionNotImplementedError(self, action)

  # screenshot_path is a helper for screenshot that generates the full path of a
  # screenshot file based on info_stack. The screenshot dir is created if
  # necessary.
  # TODO: the folder management should be done in a probe.
  def screenshot_path(self, out_dir: LocalPath, suffix: str) -> LocalPath:
    screenshot_path = out_dir / "screenshot"
    screenshot_path.mkdir(exist_ok=True)
    filename = "_".join(self.info_stack) + f"_{suffix}.png"
    return screenshot_path / filename

  # TODO: Move this into a probe, which can have multiple implementations for
  # different platforms or fullscreen vs. window, etc.
  def screenshot_impl(self, run: Run, suffix: str) -> None:
    run.browser.screenshot(self.screenshot_path(run.out_dir, suffix))

  def screenshot(self, run: Run, action: i_action.ScreenshotAction) -> None:
    del action
    self.screenshot_impl(run, "screenshot")

  def _maybe_navigate_to_about_blank(self, run: Run, page: Page) -> None:
    if duration := page.about_blank_duration:
      run.browser.show_url(run.runner, "about:blank")
      run.runner.wait(duration)

  def run_page_once(self, run: Run, page: LivePage):
    run.browser.show_url(run.runner, page.url)
    run.runner.wait(page.duration)
    self._maybe_navigate_to_about_blank(run, page)

  def run_page_multiple_tabs(self, run: Run, tabs: TabController,
                             pages: Iterable[Page]):
    # TODO: refactor possible logics to TabController.
    browser = run.browser
    for _ in tabs:
      for i, page in enumerate(pages):
        # Create a new tab for the multiple_tab case.
        if i > 0:
          browser.switch_to_new_tab()
        page.run_with(run, self, False)
      browser.switch_to_new_tab()

  def run_page(self, run: Run, page: LivePage, multiple_tabs: bool):
    if multiple_tabs:
      self.run_page_multiple_tabs(run, page.tabs, [page])
    else:
      self.run_page_once(run, page)

  def run_combined_page(self, run: Run, page: CombinedPage,
                        multiple_tabs: bool):
    if multiple_tabs:
      self.run_page_multiple_tabs(run, page.tabs, page.pages)
    else:
      for sub_page in page.pages:
        sub_page.run_with(run, self, False)

  def run_interactive_page(self, run: Run, page: InteractivePage,
                           multiple_tabs: bool):
    # TODO(lsuhua): support multiple tabs for interactive page if needed.
    if multiple_tabs:
      raise NotImplementedError(
          "Multiple tabs test for interactive page is not supported.")
    try:
      self.run_blocks(run, page.blocks)
      self._maybe_navigate_to_about_blank(run, page)
    except Exception:
      page.failure_screenshot(run)
      raise

  def run_login(self, run: Run, page: InteractivePage, login: ActionBlock):
    try:
      self.run_block(run, login)
    except Exception:
      page.failure_screenshot(run, "login-failure")
      raise
