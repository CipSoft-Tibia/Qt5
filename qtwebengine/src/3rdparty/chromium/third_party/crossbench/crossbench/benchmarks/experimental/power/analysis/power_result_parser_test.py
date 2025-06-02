# Copyright 2023 The Chromium Authors
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

from __future__ import annotations

import json
import unittest

from crossbench.benchmarks.experimental.power.analysis import power_result_parser


class PowerParserTest(unittest.TestCase):

  def setUp(self):
    super().setUp()
    self.test_battery_result_file = "./test_data/powersampler.power_battery.json"

  def test_avg_power_calculation_from_input_files_correctly(self):
    with open(self.test_battery_result_file) as f:
      power_samples = json.load(f)["data_rows"]
      self.assertLessEqual(
          abs(power_result_parser.avg_power(power_samples) - 4.822), 0.001)


if __name__ == "__main__":
  unittest.main()
