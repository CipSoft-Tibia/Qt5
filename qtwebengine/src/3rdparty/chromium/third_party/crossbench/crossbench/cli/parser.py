# Copyright 2024 The Chromium Authors
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

from __future__ import annotations

import argparse
import logging
import sys

import colorama

from crossbench.cli import ui


# Needed to gap the diff between 3.8 and 3.9 default args that change throwing
# behavior.
class _BaseCrossBenchArgumentParser(argparse.ArgumentParser):

  def fail(self, message) -> None:
    super().error(message)

  def exit(self, status=0, message=None):
    if message:
      if status == 0:
        logging.info(message)
      else:
        # Hack to get red colored output
        if ui.COLOR_LOGGING:
          print(str(colorama.Fore.RED))
        logging.critical(message)
        if ui.COLOR_LOGGING:
          print(str(colorama.Style.RESET_ALL))
    sys.exit(status)


if sys.version_info < (3, 9, 0):

  class CrossBenchArgumentParser(_BaseCrossBenchArgumentParser):

    def error(self, message) -> None:
      # Let the CrossBenchCLI handle all errors and simplify testing.
      exception = sys.exc_info()[1]
      if isinstance(exception, BaseException):
        raise exception
      raise argparse.ArgumentError(None, message)

else:

  class CrossBenchArgumentParser(_BaseCrossBenchArgumentParser):

    def __init__(self, *args, **kwargs) -> None:
      kwargs["exit_on_error"] = False
      super().__init__(*args, **kwargs)
