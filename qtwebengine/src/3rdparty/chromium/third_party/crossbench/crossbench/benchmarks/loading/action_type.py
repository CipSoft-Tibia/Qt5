# Copyright 2024 The Chromium Authors
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

from __future__ import annotations

import enum

from crossbench.config import ConfigEnum


@enum.unique
class ActionType(ConfigEnum):
  GET = ("get", "Open a URL")
  JS = ("js", "Run a custom script")
  WAIT = ("wait", "Wait for a given time")
  SCROLL = ("scroll", "Scroll on page")
  CLICK = ("click", "Click on element or at specified coordinates")
  SWIPE = ("swipe", "Swipe on screen")
  TEXT_INPUT = ("text_input", "Type printable characters at a"
                "specified speed.")
  WAIT_FOR_ELEMENT = ("wait_for_element",
                      "Wait until element appears on the page")
  INJECT_NEW_DOCUMENT_SCRIPT = ("inject_new_document_script", (
      "Evaluates given script in every frame upon creation "
      "(before loading frame's scripts). "
      "Only supported in chromium-based browsers."))
  SCREENSHOT = ("screenshot", "Take a screenshot")
