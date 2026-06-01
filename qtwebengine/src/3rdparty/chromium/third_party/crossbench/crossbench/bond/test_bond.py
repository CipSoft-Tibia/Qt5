# Copyright 2025 The Chromium Authors
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import unittest

from crossbench.bond.bond import (MAS_ONEPLATFORM_URL, MESI_APIARY_URL,
                                  AddBotsConfig, MeetLayout)
from tests import test_helper


class AddBotsConfigTestCase(unittest.TestCase):

  def test_minimal(self):
    config_dict = {"num_of_bots": 3, "ttl_secs": 123}
    config = AddBotsConfig.parse_dict(config_dict)
    config.validate()

    self.assertEqual(
        config.to_request_body_json("meet-code"), {
            "num_of_bots": 3,
            "ttl_secs": 123,
            "video_call_options": {
                "allow_vp9": True,
                "send_vp9": True,
            },
            "media_options": {
                "audio_file_path": "what_color_is_cheese_32bit_48k_stereo.raw",
                "mute_audio": False,
                "video_fps": 24,
                "mute_video": False,
                "requested_layout": str(MeetLayout.BRADY_BUNCH)
            },
            "backend_options": {
                "mesi_apiary_url": MESI_APIARY_URL,
                "mas_one_platform_url": MAS_ONEPLATFORM_URL,
            },
            "conference": {
                "conference_code": "meet-code",
            },
            "bot_type": "MEETINGS",
            "video_selection_strategy": "ROUND_ROBIN_VIDEO_SELECTION_STRATEGY",
        })

  def test_all(self):
    config_dict = {
        "num_of_bots": 4,
        "ttl_secs": 234,
        "allow_vp9": False,
        "send_vp9": False,
        "audio_file_path": "audio.raw",
        "mute_audio": True,
        "video_fps": 144,
        "mute_video": True,
        "requested_layout": MeetLayout.SPOTLIGHT,
        "video_file_path": "video.raw"
    }
    config = AddBotsConfig.parse_dict(config_dict)
    config.validate()

    self.assertEqual(
        config.to_request_body_json("meet-code"), {
            "num_of_bots": 4,
            "ttl_secs": 234,
            "video_call_options": {
                "allow_vp9": False,
                "send_vp9": False,
            },
            "media_options": {
                "audio_file_path": "audio.raw",
                "mute_audio": True,
                "video_fps": 144,
                "mute_video": True,
                "requested_layout": str(MeetLayout.SPOTLIGHT),
                "video_file_path": "video.raw",
            },
            "backend_options": {
                "mesi_apiary_url": MESI_APIARY_URL,
                "mas_one_platform_url": MAS_ONEPLATFORM_URL,
            },
            "conference": {
                "conference_code": "meet-code",
            },
            "bot_type": "MEETINGS",
        })


if __name__ == "__main__":
  test_helper.run_pytest(__file__)
