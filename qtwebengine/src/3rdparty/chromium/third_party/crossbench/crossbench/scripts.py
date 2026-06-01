# Copyright 2022 The Chromium Authors
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import sys

from crossbench.cli.btp import BTPUtil
from crossbench.cli.cli import CrossBenchCLI


def crossbench(argv=None) -> None:
  if not argv:
    argv = sys.argv
  cli = CrossBenchCLI()
  cli.run(argv[1:])


def cb_btp(argv=None) -> None:
  if not argv:
    argv = sys.argv
  btp = BTPUtil()
  btp.run(argv[1:])
