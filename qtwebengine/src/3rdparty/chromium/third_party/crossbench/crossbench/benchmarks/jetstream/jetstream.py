# Copyright 2024 The Chromium Authors
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

from __future__ import annotations

import abc

from crossbench.benchmarks.base import PressBenchmark


class JetStreamBenchmark(PressBenchmark, metaclass=abc.ABCMeta):

  @classmethod
  def short_base_name(cls) -> str:
    return "js"

  @classmethod
  def base_name(cls) -> str:
    return "jetstream"
