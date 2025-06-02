# Copyright 2023 The Chromium Authors
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

from __future__ import annotations

import argparse
import json
import statistics
from typing import List, Dict


SAMPLE_TIME = "sample_time"
EXTERNAL_CONNECTED = "battery_external_connected"
CURRENT_CAPACITY = "battery_current_capacity"
MAX_CAPACITY = "battery_max_capacity"
VOLTAGE = "battery_voltage"

def avg_power(power_samples: List[Dict[str, float]]):
  """Calculates average power consumption.

    Args:
        power_samples: List of samples of power usage. Here is an example:
                        [
                        {
                            "battery_current_capacity": 2.006,
                            "battery_external_connected": 0.0,
                            "battery_max_capacity": 4.492,
                            "battery_voltage": 11.486,
                            "sample_time": 46943039.0,
                        },
                        {
                            "battery_current_capacity": 1.999,
                            "battery_external_connected": 0.0,
                            "battery_max_capacity": 4.492,
                            "battery_voltage": 11.484,
                            "sample_time": 106941637.0,
                        }
                        ]

    Returns:
        Average power consumption in watts.
    """
  start_sample = power_samples[0]
  end_sample = power_samples[-1]
  start_consumed_ah = start_sample[MAX_CAPACITY] - start_sample[CURRENT_CAPACITY]
  end_consumed_ah = end_sample[MAX_CAPACITY] - end_sample[CURRENT_CAPACITY]
  delta_consumed_ah = start_consumed_ah - end_consumed_ah
  avg_voltage_v = statistics.fmean(sample[VOLTAGE] for sample in power_samples)
  duration_sec = (end_sample[SAMPLE_TIME] - start_sample[SAMPLE_TIME]) / 10**6
  avg_current_a = delta_consumed_ah * 3600.0 / duration_sec
  return avg_voltage_v * -avg_current_a


if __name__ == "__main__":
  parser = argparse.ArgumentParser()
  parser.add_argument(
      "battery_result_file",
      help="`powersampler.power_battery.json` file path.")
  args = parser.parse_args()

  with open(args.battery_result_file) as f:
    result_json = json.load(f)
    power_samples = result_json["data_rows"]

    if len(power_samples) < 2:
      print(
          "At least two power samples are needed to calculate power consumption."
      )
      exit(1)

    if any(sample[EXTERNAL_CONNECTED] for sample in power_samples):
      print(
          "External power was connected during the benchmark,",
          "please only use battery during benchmarking to get valid results.")
      exit(1)

    print("Average power consumption (W): {:.3f}".format(
        avg_power(power_samples)))
