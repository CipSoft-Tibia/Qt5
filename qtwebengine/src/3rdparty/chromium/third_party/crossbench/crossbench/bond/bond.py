# Copyright 2025 The Chromium Authors
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

from __future__ import annotations
import dataclasses
import enum
import logging
from typing import Any, Dict, Mapping, Optional, Sequence, Set

from google.oauth2 import service_account

import google.auth.transport.requests
from google.auth.credentials import TokenState
import requests

from crossbench.cli.config.secrets import ServiceAccount
from crossbench.config import ConfigEnum, ConfigObject, ConfigParser
from crossbench.parse import NumberParser, ObjectParser


@enum.unique
class MeetLayout(ConfigEnum):
  AUTOMATIC = ("AUTOMATIC", "Automatically pick a layout based on bot count")
  SPOTLIGHT = ("SPOTLIGHT", "Google Meet spotlight layout")
  BRADY_BUNCH = ("BRADY_BUNCH", "Google Meet brady bunch layout")
  BRADY_BUNCH_4_4 = ("BRADY_BUNCH_4_4", "Google Meet brady bunch 4x4 layout")
  BRADY_BUNCH_7_7 = ("BRADY_BUNCH_7_7", "Google Meet brady bunch 7x7 layout")


@dataclasses.dataclass(frozen=True)
class AddBotsConfig(ConfigObject):
  num_of_bots: int
  ttl_secs: int
  allow_vp9: bool
  send_vp9: bool
  audio_file_path: str
  mute_audio: bool
  video_fps: int
  mute_video: bool
  requested_layout: MeetLayout
  video_file_path: Optional[str]

  @classmethod
  def parse_str(cls, value: str) -> AddBotsConfig:
    del value
    raise NotImplementedError("Cannot create AddBotsConfig from string")

  @classmethod
  def parse_dict(cls, config: Dict) -> AddBotsConfig:
    return cls.config_parser().parse(config)

  @classmethod
  def config_parser(cls) -> ConfigParser[AddBotsConfig]:
    parser = ConfigParser(cls)
    parser.add_argument(
        "num_of_bots", type=NumberParser.positive_int, required=True)
    parser.add_argument(
        "ttl_secs", type=NumberParser.positive_int, required=True)
    parser.add_argument("allow_vp9", type=ObjectParser.bool, default=True)
    parser.add_argument("send_vp9", type=ObjectParser.bool, default=True)
    parser.add_argument(
        "audio_file_path",
        type=ObjectParser.non_empty_str,
        default="what_color_is_cheese_32bit_48k_stereo.raw")
    parser.add_argument("mute_audio", type=ObjectParser.bool, default=False)
    parser.add_argument("video_fps", type=NumberParser.positive_int, default=24)
    parser.add_argument("mute_video", type=ObjectParser.bool, default=False)
    parser.add_argument(
        "requested_layout", type=MeetLayout, default=MeetLayout.AUTOMATIC)
    parser.add_argument(
        "video_file_path", type=ObjectParser.non_empty_str, default=None)

    return parser

  def to_request_body_json(self, conference_code: str) -> Any:
    requested_layout = self.requested_layout
    if requested_layout is MeetLayout.AUTOMATIC:
      if self.num_of_bots <= 1:
        requested_layout = MeetLayout.SPOTLIGHT
      elif self.num_of_bots <= 5:
        requested_layout = MeetLayout.BRADY_BUNCH
      elif self.num_of_bots <= 15:
        requested_layout = MeetLayout.BRADY_BUNCH_4_4
      else:
        requested_layout = MeetLayout.BRADY_BUNCH_7_7

    media_options = {
        "audio_file_path": self.audio_file_path,
        "mute_audio": self.mute_audio,
        "video_fps": self.video_fps,
        "mute_video": self.mute_video,
        "requested_layout": requested_layout.value,
    }
    if self.video_file_path:
      media_options["video_file_path"] = self.video_file_path

    body_json = {
        "num_of_bots": self.num_of_bots,
        "ttl_secs": self.ttl_secs,
        "video_call_options": {
            "allow_vp9": self.allow_vp9,
            "send_vp9": self.send_vp9,
        },
        "media_options": media_options,
        "backend_options": {
            "mesi_apiary_url": MESI_APIARY_URL,
            "mas_one_platform_url": MAS_ONEPLATFORM_URL,
        },
        "conference": {
            "conference_code": conference_code,
        },
        "bot_type": "MEETINGS",
    }
    if not self.video_file_path:
      body_json[
          "video_selection_strategy"] = "ROUND_ROBIN_VIDEO_SELECTION_STRATEGY"

    return body_json


ENDPOINT = "https://bond-pa.sandbox.googleapis.com"
MESI_APIARY_URL = "https://hangouts.googleapis.com/hangouts/v1_meetings/"
MAS_ONEPLATFORM_URL = "https://preprod-meetings.googleapis.com"
SCOPE = "https://www.googleapis.com/auth/meetings"


class BondClient:
  _credentials: service_account.Credentials
  _meetings_with_bots: Set[str]

  def __init__(self, secret: ServiceAccount):
    self._credentials = service_account.Credentials.from_service_account_info(
        secret.to_json(), scopes=[SCOPE])
    self._meetings_with_bots = set()

  def _get_request_headers(self) -> Mapping[str, str]:
    if self._credentials.token_state is not TokenState.FRESH:
      self._credentials.refresh(google.auth.transport.requests.Request())
    assert self._credentials.token
    return {"Authorization": f"Bearer {self._credentials.token}"}

  def _post_with_retry(self,
                       url: str,
                       body_json: Any,
                       retry: int = 3) -> requests.Response:
    attempt = 0
    while True:
      try:
        headers = self._get_request_headers()
        response = requests.post(url, headers=headers, json=body_json)
        response.raise_for_status()
        return response
      except Exception as e:
        if attempt < retry:
          logging.warning("BondClient POST request failed, retrying: %s", e)
          attempt += 1
          continue
        raise e

  def create_meeting(self) -> str:
    request_body_json = {
        "conference_type": "THOR",
        "backend_options": {
            "mesi_apiary_url": MESI_APIARY_URL,
            "mas_one_platform_url": MAS_ONEPLATFORM_URL
        }
    }
    response = self._post_with_retry(f"{ENDPOINT}/v1/conferences:create",
                                     request_body_json)
    resonse_body_dict = ObjectParser.dict(response.json())
    conference = ObjectParser.dict(resonse_body_dict["conference"],
                                   "conference")
    conference_code = ObjectParser.non_empty_str(conference["conferenceCode"],
                                                 "conferenceCode")
    return conference_code

  def add_bots(self, conference_code: str,
               config: AddBotsConfig) -> Sequence[int]:
    request_body_json = config.to_request_body_json(conference_code)
    response = self._post_with_retry(
        f"{ENDPOINT}/v1/conference/{conference_code}/bots:add",
        request_body_json)
    response_body_dict = ObjectParser.dict(response.json())
    num_of_failures = NumberParser.positive_zero_int(
        response_body_dict["numOfFailures"], "numOfFailures")
    if num_of_failures > 0:
      raise RuntimeError(f"{num_of_failures} failures when adding bots")
    bot_ids = ObjectParser.sequence(response_body_dict["botIds"])
    for bot_id in bot_ids:
      NumberParser.any_int(bot_id)
    self._meetings_with_bots.add(conference_code)
    return bot_ids

  def remove_all_bots(self, conference_code: str):
    request_body_json = {
        "conference": {
            "conference_code": conference_code,
        },
        "bot_type": "MEETINGS",
        "remove_all": True,
    }
    self._post_with_retry(
        f"{ENDPOINT}/v1/conference/{conference_code}/bots:remove",
        request_body_json)

  def teardown(self):
    for conference_code in self._meetings_with_bots:
      self.remove_all_bots(conference_code)
    self._meetings_with_bots = set()
