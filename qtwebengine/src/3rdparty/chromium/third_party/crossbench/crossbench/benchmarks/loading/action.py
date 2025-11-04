# Copyright 2023 The Chromium Authors
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

from __future__ import annotations

import abc
import datetime as dt
import enum
import logging
from typing import TYPE_CHECKING, Any, Dict, Optional, Tuple, Type, TypeVar

from crossbench import cli_helper, exception
from crossbench.benchmarks.loading.action_runner.base import ActionRunner
from crossbench.benchmarks.loading.action_type import ActionType
from crossbench.benchmarks.loading.input_source import InputSource
from crossbench.benchmarks.loading.point import Point
from crossbench.config import ConfigEnum, ConfigObject, ConfigParser

if TYPE_CHECKING:
  import crossbench.path as pth
  from crossbench.runner.run import Run
  from crossbench.types import JsonDict


class ActionTypeConfigParser(ConfigParser):
  """Custom ConfigParser for ActionType that works on
  Action Configs. This way we can pop the 'value' or 'type' key from the
  config dict."""

  def __init__(self):
    super().__init__("ActionType parser", ActionType)
    self.add_argument(
        "action",
        aliases=("type",),
        type=cli_helper.parse_non_empty_str,
        required=True)

  def new_instance_from_kwargs(self, kwargs: Dict[str, Any]) -> ActionType:
    return ActionType(kwargs["action"])


_ACTION_TYPE_CONFIG_PARSER = ActionTypeConfigParser()


@enum.unique
class ButtonClick(ConfigEnum):
  LEFT = ("left", "Press left mouse button")
  RIGHT = ("right", "Press right mouse button")
  MIDDLE = ("middle", "Press middle mouse button")


ACTION_TIMEOUT = dt.timedelta(seconds=20)

ActionT = TypeVar("ActionT", bound="Action")


class Action(ConfigObject, metaclass=abc.ABCMeta):
  TYPE: ActionType = ActionType.GET

  @classmethod
  def parse_str(cls, value: str) -> GetAction:
    return GetAction.parse_str(value)

  @classmethod
  def parse_dict(cls: Type[ActionT], config: Dict[str, Any]) -> ActionT:
    action_type: ActionType = _ACTION_TYPE_CONFIG_PARSER.parse(config)
    action_cls: Type[ActionT] = ACTIONS[action_type]
    with exception.annotate_argparsing(
        f"Parsing Action details  ...{{ action: \"{action_type}\", ...}}:"):
      action = action_cls.config_parser().parse(config)
    assert isinstance(action, cls), f"Expected {cls} but got {type(action)}"
    return action

  @classmethod
  def config_parser(cls: Type[ActionT]) -> ConfigParser[ActionT]:
    parser = ConfigParser(f"{cls.__name__} parser", cls)
    parser.add_argument(
        "index",
        type=cli_helper.parse_positive_zero_int,
        required=False,
        default=0)
    parser.add_argument(
        "timeout",
        type=cli_helper.Duration.parse_non_zero,
        default=ACTION_TIMEOUT)
    return parser

  def __init__(self, timeout: dt.timedelta = ACTION_TIMEOUT, index: int = 0):
    self._timeout: dt.timedelta = timeout
    self._index = index
    self.validate()

  @property
  def index(self) -> int:
    return self._index

  @property
  def duration(self) -> dt.timedelta:
    return dt.timedelta(milliseconds=10)

  @property
  def timeout(self) -> dt.timedelta:
    return self._timeout

  @property
  def has_timeout(self) -> bool:
    return self._timeout != dt.timedelta.max

  @abc.abstractmethod
  def run_with(self, run: Run, action_runner: ActionRunner) -> None:
    pass

  def validate(self) -> None:
    if self._timeout.total_seconds() < 0:
      raise ValueError(
          f"{self}.timeout should be positive, but got {self.timeout}")

  def to_json(self) -> JsonDict:
    return {"type": str(self.TYPE), "timeout": self.timeout.total_seconds()}

  def __str__(self) -> str:
    return type(self).__name__

  def __eq__(self, other: object) -> bool:
    if isinstance(other, Action):
      return self.to_json() == other.to_json()
    return False


@enum.unique
class ReadyState(ConfigEnum):
  """See https://developer.mozilla.org/en-US/docs/Web/API/Document/readyState"""
  # Non-blocking:
  ANY = ("any", "Ignore ready state")
  # Blocking (on dom event):
  LOADING = ("loading", "The document is still loading.")
  INTERACTIVE = ("interactive", "The document has finished loading "
                 "but sub-resources might still be loading")
  COMPLETE = ("complete",
              "The document and all sub-resources have finished loading.")


@enum.unique
class WindowTarget(ConfigEnum):
  """See https://developer.mozilla.org/en-US/docs/Web/API/Window/open"""
  SELF = ("_self", "The current browsing context. (Default)")
  BLANK = ("_blank", "Usually a new tab, but users can configure browsers "
           "to open a new window instead.")
  PARENT = ("_parent", "The parent browsing context of the current one. "
            "If no parent, behaves as _self.")
  TOP = ("_top", "The topmost browsing context "
         "(the 'highest' context that's an ancestor of the current one). "
         "If no ancestors, behaves as _self.")


class BaseDurationAction(Action):

  def __init__(self,
               duration: dt.timedelta,
               timeout: dt.timedelta = ACTION_TIMEOUT,
               index: int = 0) -> None:
    self._duration: dt.timedelta = duration
    super().__init__(timeout, index)

  @property
  def duration(self) -> dt.timedelta:
    return self._duration

  def validate(self) -> None:
    super().validate()
    self.validate_duration()

  def validate_duration(self) -> None:
    if self.duration.total_seconds() <= 0:
      raise ValueError(
          f"{self}.duration should be positive, but got {self.duration}")

  def to_json(self) -> JsonDict:
    details = super().to_json()
    details["duration"] = self.duration.total_seconds()
    return details


class InputSourceAction(BaseDurationAction, metaclass=abc.ABCMeta):

  @classmethod
  def config_parser(cls: Type[ActionT]) -> ConfigParser[ActionT]:
    parser = super().config_parser()
    parser.add_argument(
        "source", type=InputSource.parse, default=InputSource.JS)
    return parser

  def __init__(self,
               source: InputSource,
               duration: dt.timedelta,
               timeout: dt.timedelta = ACTION_TIMEOUT,
               index: int = 0) -> None:
    self._input_source = source
    super().__init__(duration, timeout, index)

  @property
  def input_source(self) -> InputSource:
    return self._input_source

  def validate(self) -> None:
    super().validate()
    self.validate_input_source()

  def validate_input_source(self) -> None:
    if self.input_source not in self.supported_input_sources():
      raise ValueError(
          f"Unsupported input source for {self.__class__.__name__}")

  @abc.abstractmethod
  def supported_input_sources(self) -> Tuple[InputSource, ...]:
    pass

  def to_json(self) -> JsonDict:
    details = super().to_json()
    details["source"] = self.input_source
    return details


class GetAction(BaseDurationAction):
  TYPE: ActionType = ActionType.GET

  @classmethod
  def parse_str(cls, value: str) -> GetAction:
    return cls(url=cli_helper.parse_fuzzy_url_str(value))

  @classmethod
  def config_parser(cls: Type[ActionT]) -> ConfigParser[ActionT]:
    parser = super().config_parser()
    parser.add_argument(
        "url", type=cli_helper.parse_fuzzy_url_str, required=True)
    parser.add_argument(
        "duration", type=cli_helper.Duration.parse_zero, default=dt.timedelta())
    parser.add_argument(
        "ready_state", type=ReadyState.parse, default=ReadyState.ANY)
    parser.add_argument(
        "target", type=WindowTarget.parse, default=WindowTarget.SELF)
    return parser

  def __init__(self,
               url: str,
               duration: dt.timedelta = dt.timedelta(),
               timeout: dt.timedelta = ACTION_TIMEOUT,
               ready_state: ReadyState = ReadyState.ANY,
               target: WindowTarget = WindowTarget.SELF,
               index: int = 0):
    if not url:
      raise ValueError(f"{self}.url is missing")
    self._url: str = url
    self._ready_state = ready_state
    self._target = target
    super().__init__(duration, timeout, index)

  def validate_duration(self) -> None:
    if self.ready_state != ReadyState.ANY:
      if self.duration != dt.timedelta():
        raise ValueError(
            f"Expected empty duration with ReadyState {self.ready_state} "
            f"but got: {self.duration}")
      self._duration = dt.timedelta()

  @property
  def url(self) -> str:
    return self._url

  @property
  def ready_state(self) -> ReadyState:
    return self._ready_state

  @property
  def duration(self) -> dt.timedelta:
    return self._duration

  @property
  def target(self) -> WindowTarget:
    return self._target

  def run_with(self, run: Run, action_runner: ActionRunner) -> None:
    action_runner.get(run, self)

  def to_json(self) -> JsonDict:
    details = super().to_json()
    details["url"] = self.url
    details["ready_state"] = str(self.ready_state)
    details["target"] = str(self.target)
    return details


class DurationAction(BaseDurationAction):
  TYPE: ActionType = ActionType.WAIT

  @classmethod
  def config_parser(cls: Type[ActionT]) -> ConfigParser[ActionT]:
    parser = super().config_parser()
    parser.add_argument(
        "duration", type=cli_helper.Duration.parse_non_zero, required=True)
    return parser


class WaitAction(DurationAction):
  TYPE: ActionType = ActionType.WAIT

  def run_with(self, run: Run, action_runner: ActionRunner) -> None:
    action_runner.wait(run, self)


class ScrollAction(InputSourceAction):
  TYPE: ActionType = ActionType.SCROLL

  @classmethod
  def config_parser(cls: Type[ActionT]) -> ConfigParser[ActionT]:
    parser = super().config_parser()
    parser.add_argument("distance", type=cli_helper.parse_float, default=500)
    parser.add_argument(
        "duration",
        type=cli_helper.Duration.parse_non_zero,
        default=dt.timedelta(seconds=1))
    parser.add_argument("selector", type=cli_helper.parse_non_empty_str)
    parser.add_argument("required", type=cli_helper.parse_bool, default=False)
    return parser

  def __init__(self,
               source: InputSource,
               distance: float = 500.0,
               duration: dt.timedelta = dt.timedelta(seconds=1),
               selector: Optional[str] = None,
               required: bool = False,
               timeout: dt.timedelta = ACTION_TIMEOUT,
               index: int = 0) -> None:
    self._distance = distance

    # TODO: convert to custom selector object.
    self._selector = selector
    self._required = required
    super().__init__(source, duration, timeout, index)

  @property
  def distance(self) -> float:
    return self._distance

  @property
  def selector(self) -> Optional[str]:
    return self._selector

  @property
  def required(self) -> bool:
    return self._required

  def run_with(self, run: Run, action_runner: ActionRunner) -> None:
    action_runner.scroll(run, self)

  def validate(self) -> None:
    super().validate()
    if not self.distance:
      raise ValueError(f"{self}.distance is not provided")

    if self.required and not self.selector:
      raise ValueError(
          "'required' can only be used when a selector is specified")

  def supported_input_sources(self) -> Tuple[InputSource, ...]:
    return (InputSource.JS, InputSource.TOUCH)

  def to_json(self) -> JsonDict:
    details = super().to_json()
    details["distance"] = str(self.distance)
    return details


class ClickAction(InputSourceAction):
  TYPE: ActionType = ActionType.CLICK

  @classmethod
  def config_parser(cls: Type[ActionT]) -> ConfigParser[ActionT]:
    parser = super().config_parser()
    parser.add_argument("selector", type=cli_helper.parse_non_empty_str)
    parser.add_argument("required", type=cli_helper.parse_bool, default=False)
    parser.add_argument(
        "scroll_into_view", type=cli_helper.parse_bool, default=False)
    parser.add_argument("x", type=cli_helper.parse_positive_zero_int)
    parser.add_argument("y", type=cli_helper.parse_positive_zero_int)
    parser.add_argument(
        "duration", type=cli_helper.Duration.parse_zero, default=dt.timedelta())
    return parser

  def __init__(self,
               source: InputSource,
               duration: dt.timedelta = dt.timedelta(),
               selector: Optional[str] = None,
               required: bool = False,
               scroll_into_view: bool = False,
               x: Optional[int] = None,
               y: Optional[int] = None,
               timeout: dt.timedelta = ACTION_TIMEOUT,
               index: int = 0):
    # TODO: convert to custom selector object.
    self._selector = selector
    self._required: bool = required
    self._scroll_into_view: bool = scroll_into_view
    self._coordinates: Optional[Point] = None
    if x is not None and y is not None:
      self._coordinates = Point(x, y)
    super().__init__(source, duration, timeout, index)

  @property
  def selector(self) -> Optional[str]:
    return self._selector

  @property
  def required(self) -> bool:
    return self._required

  @property
  def scroll_into_view(self) -> bool:
    return self._scroll_into_view

  @property
  def coordinates(self) -> Optional[Point]:
    return self._coordinates

  def run_with(self, run: Run, action_runner: ActionRunner) -> None:
    action_runner.click(run, self)

  def validate(self) -> None:
    super().validate()

    if self._selector and self._coordinates:
      raise ValueError("Only one is allowed: either selector or coordinates")

    if not self._selector and not self._coordinates:
      raise ValueError("Either selector or coordinates are required")

    if self._input_source is InputSource.JS and self._coordinates:
      raise ValueError("X,Y Coordinates cannot be used with JS click source.")

    if self._required and self._coordinates:
      raise ValueError("'required' is not compatible with coordinates")

    if self._scroll_into_view and self._coordinates:
      raise ValueError("'scroll_into_view' is not compatible with coordinates")

  def validate_duration(self) -> None:
    # A click action is allowed to have a zero duration.
    return

  def supported_input_sources(self) -> Tuple[InputSource, ...]:
    return (InputSource.JS, InputSource.TOUCH, InputSource.MOUSE)

  def to_json(self) -> JsonDict:
    details = super().to_json()

    if self._selector:
      details["selector"] = self._selector
      details["required"] = self._required
      details["scroll_into_view"] = self._scroll_into_view
    else:
      assert self._coordinates
      details["x"] = self._coordinates.x
      details["y"] = self._coordinates.y
    return details


class SwipeAction(DurationAction):
  TYPE: ActionType = ActionType.SWIPE

  @classmethod
  def config_parser(cls: Type[ActionT]) -> ConfigParser[ActionT]:
    parser = super().config_parser()
    parser.add_argument(
        "start_x",
        aliases=("startx",),
        type=cli_helper.parse_int,
        required=True)
    parser.add_argument(
        "start_y",
        aliases=("starty",),
        type=cli_helper.parse_int,
        required=True)
    parser.add_argument(
        "end_x", aliases=("endx",), type=cli_helper.parse_int, required=True)
    parser.add_argument(
        "end_y", aliases=("endy",), type=cli_helper.parse_int, required=True)
    return parser

  def __init__(self,
               start_x: int,
               start_y: int,
               end_x: int,
               end_y: int,
               duration: dt.timedelta = dt.timedelta(seconds=1),
               timeout: dt.timedelta = ACTION_TIMEOUT,
               index: int = 0) -> None:
    self._start_x: int = start_x
    self._start_y: int = start_y
    self._end_x: int = end_x
    self._end_y: int = end_y
    super().__init__(duration, timeout, index)

  @property
  def start_x(self) -> int:
    return self._start_x

  @property
  def start_y(self) -> int:
    return self._start_y

  @property
  def end_x(self) -> int:
    return self._end_x

  @property
  def end_y(self) -> int:
    return self._end_y

  def run_with(self, run: Run, action_runner: ActionRunner) -> None:
    action_runner.swipe(run, self)

  def to_json(self) -> JsonDict:
    details = super().to_json()
    details["start_x"] = self._start_x
    details["start_y"] = self._start_y
    details["end_x"] = self._end_x
    details["end_y"] = self._end_y
    return details


class TextInputAction(InputSourceAction):
  TYPE: ActionType = ActionType.TEXT_INPUT

  @classmethod
  def config_parser(cls: Type[ActionT]) -> ConfigParser[ActionT]:
    parser = super().config_parser()
    parser.add_argument(
        "text", type=cli_helper.parse_non_empty_str, required=True)
    parser.add_argument(
        "duration", type=cli_helper.Duration.parse_zero, default=dt.timedelta())
    return parser

  def __init__(self,
               source: InputSource,
               duration: dt.timedelta,
               text: str,
               timeout: dt.timedelta = ACTION_TIMEOUT,
               index: int = 0) -> None:
    self._text: str = text
    super().__init__(source, duration, timeout, index)

  @property
  def text(self) -> str:
    return self._text

  def run_with(self, run: Run, action_runner: ActionRunner) -> None:
    action_runner.text_input(run, self)

  def validate(self) -> None:
    super().validate()
    if not self._text:
      raise ValueError(f"{self}.text is missing.")

  def validate_duration(self) -> None:
    # A text input action is allowed to have a zero duration.
    return

  def supported_input_sources(self) -> Tuple[InputSource, ...]:
    return (InputSource.JS, InputSource.KEYBOARD)

  def to_json(self) -> JsonDict:
    details = super().to_json()
    details["text"] = self._text
    return details


class WaitForElementAction(Action):
  TYPE: ActionType = ActionType.WAIT_FOR_ELEMENT

  @classmethod
  def config_parser(cls: Type[ActionT]) -> ConfigParser[ActionT]:
    parser = super().config_parser()
    parser.add_argument(
        "selector", type=cli_helper.parse_non_empty_str, required=True)
    return parser

  def __init__(self,
               selector: str,
               timeout: dt.timedelta = ACTION_TIMEOUT,
               index: int = 0):
    self._selector = selector
    super().__init__(timeout, index)

  @property
  def selector(self) -> str:
    return self._selector

  def run_with(self, run: Run, action_runner: ActionRunner) -> None:
    action_runner.wait_for_element(run, self)

  def validate(self) -> None:
    super().validate()
    if not self.selector:
      raise ValueError(f"{self}.selector is missing.")

  def to_json(self) -> JsonDict:
    details = super().to_json()
    details["selector"] = self.selector
    return details


def parse_replacement_dict(value: Any) -> Dict[str, str]:
  dict_value = cli_helper.parse_dict(value)
  for replace_key, replace_value in dict_value.items():
    with exception.annotate_argparsing(
        f"Parsing ...[{repr(replace_key)}] = {repr(value)}"):
      cli_helper.parse_non_empty_str(replace_key, "replacement key")
      cli_helper.parse_str(replace_value, "replacement value")
  return dict_value


class JsAction(Action):
  TYPE: ActionType = ActionType.JS

  @classmethod
  def config_parser(cls: Type[ActionT]) -> ConfigParser[ActionT]:
    parser = super().config_parser()
    parser.add_argument("script", type=cli_helper.parse_non_empty_str)
    parser.add_argument(
        "script_path",
        aliases=("path",),
        type=cli_helper.parse_existing_file_path)
    parser.add_argument(
        "replacements", aliases=("replace",), type=parse_replacement_dict)
    return parser

  def __init__(self,
               script: Optional[str],
               script_path: Optional[pth.LocalPath],
               replacements: Optional[Dict[str, str]] = None,
               timeout: dt.timedelta = ACTION_TIMEOUT,
               index: int = 0) -> None:
    self._original_script = script
    self._script_path = script_path
    self._script = ""
    if bool(script) == bool(script_path):
      raise ValueError(
          f"One of {self}.script or {self}.script_path, but not both, "
          "have to specified. ")
    if script:
      self._script = script
    elif script_path:
      self._script = script_path.read_text()
      logging.debug("Loading script from %s: %s", script_path, script)
      # TODO: support argument injection into shared file script.
    self._replacements = replacements
    if replacements:
      for key, value in replacements.items():
        self._script = self._script.replace(key, value)
    super().__init__(timeout, index)

  @property
  def script(self) -> str:
    return self._script

  def run_with(self, run: Run, action_runner: ActionRunner) -> None:
    action_runner.js(run, self)

  def validate(self) -> None:
    super().validate()
    if not self.script:
      raise ValueError(
          f"{self}.script is missing or the provided script file is empty.")

  def to_json(self) -> JsonDict:
    details = super().to_json()
    if self._original_script:
      details["script"] = self._original_script
    if self._script_path:
      details["script_path"] = str(self._script_path)
    if self._replacements:
      details["replacements"] = self._replacements
    return details


class InjectNewDocumentScriptAction(JsAction):
  TYPE: ActionType = ActionType.INJECT_NEW_DOCUMENT_SCRIPT

  def run_with(self, run: Run, action_runner: ActionRunner) -> None:
    action_runner.inject_new_document_script(run, self)


class ScreenshotAction(Action):
  TYPE: ActionType = ActionType.SCREENSHOT

  def __init__(self,
               timeout: dt.timedelta = ACTION_TIMEOUT,
               index: int = 0) -> None:
    super().__init__(timeout, index)

  def run_with(self, run: Run, action_runner: ActionRunner) -> None:
    action_runner.screenshot(run, self)


ACTIONS_TUPLE: Tuple[Type[Action], ...] = (
    ClickAction,
    GetAction,
    InjectNewDocumentScriptAction,
    JsAction,
    ScreenshotAction,
    ScrollAction,
    SwipeAction,
    TextInputAction,
    WaitAction,
    WaitForElementAction,
)

ACTIONS: Dict[ActionType, Type] = {
    action_cls.TYPE: action_cls for action_cls in ACTIONS_TUPLE
}

assert len(ACTIONS_TUPLE) == len(ACTIONS), "Non unique Action.TYPE present"
