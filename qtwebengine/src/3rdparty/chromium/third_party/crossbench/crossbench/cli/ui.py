# Copyright 2024 The Chromium Authors
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import logging

import colorama

colorama.init()

COLOR_LOGGING: bool = True


class ColoredLogFormatter(logging.Formatter):

  FORMAT = "%(message)s"

  FORMATS = {
      logging.DEBUG:
          FORMAT,
      logging.INFO:
          str(colorama.Fore.GREEN) + FORMAT + str(colorama.Fore.RESET),
      logging.WARNING:
          str(colorama.Fore.YELLOW) + FORMAT + str(colorama.Fore.RESET),
      logging.ERROR:
          str(colorama.Fore.RED) + FORMAT + str(colorama.Fore.RESET),
      logging.CRITICAL:
          str(colorama.Style.BRIGHT) + FORMAT + str(colorama.Style.RESET_ALL),
  }

  def format(self, record: logging.LogRecord) -> str:
    log_fmt = self.FORMATS.get(record.levelno)
    formatter = logging.Formatter(log_fmt)
    return formatter.format(record)

  def formatException(self, ei):
    return ""

  def formatStack(self, stack_info):
    return ""
