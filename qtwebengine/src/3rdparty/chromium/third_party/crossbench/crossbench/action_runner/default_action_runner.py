# Copyright 2024 The Chromium Authors
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

from __future__ import annotations

import datetime as dt
import logging
import time
from typing import TYPE_CHECKING, Any, Callable, Optional, Sequence, Tuple

from crossbench.action_runner.action import all as i_action
from crossbench.action_runner.default_bond_action_runner import (
    DefaultBondActionRunner)
from crossbench.action_runner.action.enums import ReadyState
from crossbench.action_runner.base import (ActionRunner,
                                           InputSourceNotImplementedError)
from crossbench.action_runner.element_not_found_error import \
    ElementNotFoundError
from crossbench.action_runner.screenshot_annotation import ScreenshotAnnotation
from crossbench.probes.dump_html import DumpHtmlProbe, DumpHtmlProbeContext
from crossbench.probes.screenshot import (ScreenshotProbe,
                                          ScreenshotProbeContext)

if TYPE_CHECKING:
  from crossbench.action_runner.bond_base import BondActionRunner
  from crossbench.runner.actions import Actions
  from crossbench.runner.run import Run


class DefaultActionRunner(ActionRunner):
  XPATH_SELECT_ELEMENT = """
      let element = document.evaluate(arguments[0], document).iterateNext();
  """

  CSS_SELECT_ELEMENT = """
      let element = document.querySelector(arguments[0]);
  """

  CHECK_ELEMENT_EXISTS = """
      if (!element) return false;
  """

  ELEMENT_SCROLL_INTO_VIEW = """
      element.scrollIntoView();
  """

  CHECK_ELEMENT_RECT = """
      const rect = element.getBoundingClientRect();
      if (rect.width === 0 || rect.height === 0) return false;
  """

  ELEMENT_CLICK = """
      element.click();
  """

  RETURN_SUCCESS = """
      return true;
  """

  SELECT_WINDOW = """
      let element = window;
  """

  SCROLL_ELEMENT_TO = """
      element.scrollTo({top:arguments[1], behavior:'smooth'});
  """

  GET_CURRENT_SCROLL_POSITION = """
      if (!element) return [false, 0];
      return [true, element[arguments[1]]];
  """

  _bond: Optional[DefaultBondActionRunner] = None

  def get_selector_script(self,
                          selector: str,
                          check_element_exists=False,
                          scroll_into_view=False,
                          check_element_rect=False,
                          click=False,
                          return_on_success=False) -> Tuple[str, str]:
    # TODO: support more selector types

    script: str = ""

    prefix = "xpath/"
    if selector.startswith(prefix):
      selector = selector[len(prefix):]
      script = self.XPATH_SELECT_ELEMENT
    else:
      script = self.CSS_SELECT_ELEMENT

    if check_element_exists:
      script += self.CHECK_ELEMENT_EXISTS

    if scroll_into_view:
      script += self.ELEMENT_SCROLL_INTO_VIEW

    if check_element_rect:
      script += self.CHECK_ELEMENT_RECT

    if click:
      script += self.ELEMENT_CLICK

    if return_on_success:
      script += self.RETURN_SUCCESS

    return selector, script

  @property
  def bond(self) -> BondActionRunner:
    if not self._bond:
      self._bond = DefaultBondActionRunner(self)
    return self._bond

  def _wait_for_ready_state(self, actions: Actions, ready_state: ReadyState,
                            timeout: dt.timedelta) -> None:
    # Make sure we also finish if readyState jumps directly
    # from "loading" to "complete"
    actions.wait_js_condition(
        f"""
          let state = document.readyState;
          return state === '{ready_state}' || state === "complete";
        """, 0.2, timeout.total_seconds())

  def teardown(self, run: Run):
    del run
    if self._bond:
      self._bond.teardown()

  def get(self, run: Run, action: i_action.GetAction) -> None:
    # TODO: potentially refactor the timing and logging out to the base class.
    start_time = time.time()
    expected_end_time = start_time + action.duration.total_seconds()

    with run.actions(f"Get {action.url}", measure=False) as actions:
      actions.show_url(action.url, str(action.target))

      if action.ready_state != ReadyState.ANY:
        self._wait_for_ready_state(actions, action.ready_state, action.timeout)
        return
      # Wait for the given duration from the start of the action.
      wait_time_seconds = expected_end_time - time.time()
      if wait_time_seconds > 0:
        actions.wait(wait_time_seconds)
      elif action.duration:
        run_duration = dt.timedelta(seconds=time.time() - start_time)
        logging.info("%s took longer (%s) than expected action duration (%s).",
                     action, run_duration, action.duration)

  def click_js(self, run: Run, action: i_action.ClickAction) -> None:

    if action.duration > dt.timedelta():
      raise InputSourceNotImplementedError(self, action, action.input_source,
                                           "Non-zero duration not implemented")
    selector_config = action.position.selector
    if not selector_config:
      raise RuntimeError("Missing selector")

    selector, script = self.get_selector_script(
        selector_config.selector,
        check_element_exists=True,
        scroll_into_view=selector_config.scroll_into_view,
        click=True,
        return_on_success=True)

    with run.actions("ClickAction", measure=False) as actions:
      if selector_config.wait:
        self.wait_for_element_impl(
            actions, selector=selector_config.selector, timeout=action.timeout)
      if not actions.js(
          script, arguments=[selector]) and selector_config.required:
        raise ElementNotFoundError(selector)

      if action.verify:
        self.wait_for_element_impl(
            actions, selector=action.verify, timeout=action.timeout)

  def scroll_js(self, run: Run, action: i_action.ScrollAction) -> None:
    with run.actions("ScrollAction", measure=False) as actions:
      selector = ""
      selector_script = self.SELECT_WINDOW

      if action.selector:
        selector, selector_script = self.get_selector_script(action.selector)

      current_scroll_position_script = (
          selector_script + self.GET_CURRENT_SCROLL_POSITION)

      found_element, initial_scroll_y = actions.js(
          current_scroll_position_script,
          arguments=[selector,
                     self._get_scroll_field(bool(action.selector))])

      if not found_element:
        if action.required:
          raise ElementNotFoundError(selector)
        return

      do_scroll_script = selector_script + self.SCROLL_ELEMENT_TO

      duration_s = action.duration.total_seconds()
      distance = action.distance

      start_time = time.time()
      # TODO: use the chrome.gpuBenchmarking.smoothScrollBy extension
      # if available.
      while True:
        time_delta = time.time() - start_time
        if time_delta >= duration_s:
          break
        scroll_y = initial_scroll_y + time_delta / duration_s * distance
        actions.js(do_scroll_script, arguments=[selector, scroll_y])
        actions.wait(0.2)
      scroll_y = initial_scroll_y + distance
      actions.js(do_scroll_script, arguments=[selector, scroll_y])

  def wait_for_element_impl(self,
                            actions: Actions,
                            selector: str,
                            timeout: dt.timedelta,
                            scroll_into_view: bool = False,
                            check_element_rect: bool = False) -> None:
    selector, selector_script = self.get_selector_script(
        selector=selector,
        check_element_exists=True,
        scroll_into_view=scroll_into_view,
        check_element_rect=check_element_rect,
        return_on_success=True)
    # TODO: if check_element_rect, we should wait for the position to be the
    # same
    actions.wait_js_condition(
        selector_script, min_wait=0.2, timeout=timeout, arguments=(selector,))

  def wait_for_element(self, run: Run,
                       action: i_action.WaitForElementAction) -> None:
    with run.actions("WaitForElementAction", measure=False) as actions:
      self.wait_for_element_impl(
          actions=actions, selector=action.selector, timeout=action.timeout)

  def wait_for_ready_state(self, run: Run,
                           action: i_action.WaitForReadyStateAction) -> None:
    with run.actions(
        f"Wait for ready state {action.ready_state}", measure=False) as actions:
      self._wait_for_ready_state(actions, action.ready_state, action.timeout)

  def inject_new_document_script(
      self, run: Run, action: i_action.InjectNewDocumentScriptAction) -> None:
    run.browser.run_script_on_new_document(action.script)

  def switch_tab(self, run: Run, action: i_action.SwitchTabAction) -> None:
    with run.actions("SwitchTabAction", measure=False):
      run.browser.switch_tab(action.title, action.url, action.tab_index,
                             action.timeout)

  def _get_scroll_field(self, has_selector: bool) -> str:
    if has_selector:
      return "scrollTop"
    return "scrollY"

  def _rate_limit_keystrokes(
      self, run: Run, action: i_action.TextInputAction,
      do_type_function: Callable[[Run, Actions, str], Any]) -> None:
    character_delay_s = (action.duration / len(action.text)).total_seconds()
    start_time = time.time()
    action_expected_end_time = start_time + action.duration.total_seconds()

    with run.actions("TextInput", measure=False) as actions:

      # When no duration is specified, input the entire text at once.
      if action.duration == dt.timedelta():
        do_type_function(run, actions, action.text)
        return

      character_expected_end_time = start_time

      for character in action.text:
        character_expected_end_time += character_delay_s

        do_type_function(run, actions, character)

        expected_end_delta = character_expected_end_time - time.time()

        if expected_end_delta > 0:
          actions.wait(expected_end_delta)

      overrun_time = time.time() - action_expected_end_time

      # There will always be a slight overrun due to the overhead of the final
      # actions.wait() call, but that is acceptable. Check if the overrun was
      # significant.
      if overrun_time > 0.01:
        logging.warning(
            "text_input action is behind schedule! Consider extending this "
            "action's duration otherwise the action may timeout.")

  def screenshot_impl(
      self,
      run: Run,
      suffix: str,
      annotations: Optional[Sequence[ScreenshotAnnotation]] = None) -> None:
    ctx = run.find_probe_context(ScreenshotProbe)
    if not ctx:
      logging.warning("No screenshot probe for screenshot on %s",
                      repr(self.info_stack))
      return
    assert isinstance(ctx, ScreenshotProbeContext)
    ctx.screenshot("_".join(self.info_stack) + f"_{suffix}", annotations)

  def dump_html_impl(self, run: Run, suffix: str) -> None:
    ctx = run.find_probe_context(DumpHtmlProbe)
    if not ctx:
      logging.warning("No dump_html probe for dump on %s",
                      repr(self.info_stack))
      return
    assert isinstance(ctx, DumpHtmlProbeContext)
    ctx.dump_html("_".join(self.info_stack) + f"_{suffix}")
