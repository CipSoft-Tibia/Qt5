# Copyright 2024 The Chromium Authors
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

from __future__ import annotations

import sys
import threading
import time
from typing import Iterable


class Spinner:
  CURSORS = "◐◓◑◒"

  def __init__(self, sleep: float = 0.5) -> None:
    self._is_running = False
    self._sleep_time = sleep

  def __enter__(self) -> None:
    # Only enable the spinner if the output is an interactive terminal.
    is_atty = hasattr(sys.stdout, "isatty") and sys.stdout.isatty()
    if is_atty:
      self._is_running = True
      threading.Thread(target=self._spin).start()

  def __exit__(self, exc_type, exc_value, traceback) -> None:
    if self._is_running:
      self._is_running = False
      self._sleep()

  def _cursors(self) -> Iterable[str]:
    while True:
      yield from Spinner.CURSORS

  def _spin(self) -> None:
    stdout = sys.stdout
    for cursor in self._cursors():
      if not self._is_running:
        return
      # Print the current wait-cursor and send a carriage return to move to the
      # start of the line.
      stdout.write(f" {cursor}\r")
      stdout.flush()
      self._sleep()

  def _sleep(self) -> None:
    time.sleep(self._sleep_time)
