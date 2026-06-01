# Copyright 2024 The Chromium Authors
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

from __future__ import annotations

import dataclasses
from typing import TYPE_CHECKING, Dict, Optional

from crossbench.config import ConfigObject, ConfigParser
from crossbench.parse import ObjectParser

if TYPE_CHECKING:
  from crossbench.types import JsonDict


@dataclasses.dataclass(frozen=True)
class Secrets(ConfigObject):
  google: Optional[UsernamePassword] = None
  bond: Optional[ServiceAccount] = None

  @classmethod
  def config_parser(cls) -> ConfigParser[Secrets]:
    parser = ConfigParser(cls)
    parser.add_argument("google", type=GoogleUsernamePassword)
    parser.add_argument("bond", type=ServiceAccount)
    return parser

  @classmethod
  def parse_str(cls, value: str) -> Secrets:
    if value[0] == "{":
      return cls.parse_inline_hjson(value)
    raise NotImplementedError("Cannot create secrets from string")

  @classmethod
  def parse_dict(cls, config: Dict) -> Secrets:
    return cls.config_parser().parse(config)

  def merge(self, fallback: Secrets) -> Secrets:
    return Secrets(self.google or fallback.google, self.bond or fallback.bond)

@dataclasses.dataclass(frozen=True)
class UsernamePassword(ConfigObject):
  username: str
  password: str

  @classmethod
  def config_parser(cls) -> ConfigParser[UsernamePassword]:
    parser = ConfigParser(cls)
    parser.add_argument(
        "username",
        aliases=("user", "usr", "account"),
        type=ObjectParser.non_empty_str,
        required=True)
    parser.add_argument(
        "password",
        aliases=("pass", "pw"),
        type=ObjectParser.any_str,
        required=True)
    return parser

  @classmethod
  def parse_dict(cls, config: Dict) -> UsernamePassword:
    return cls.config_parser().parse(config)

  @classmethod
  def parse_str(cls, value: str):
    # TODO: maybe support passwd style string format
    raise NotImplementedError("Cannot support")


class GoogleUsernamePassword(UsernamePassword):
  pass


@dataclasses.dataclass(frozen=True)
class ServiceAccount(ConfigObject):
  type: str
  project_id: str
  private_key_id: str
  private_key: str
  client_email: str
  client_id: str
  auth_uri: str
  token_uri: str
  auth_provider_x509_cert_url: str
  client_x509_cert_url: str
  universe_domain: str

  @classmethod
  def config_parser(cls) -> ConfigParser[ServiceAccount]:
    parser = ConfigParser(cls)
    parser.add_argument("type", type=ObjectParser.non_empty_str, required=True)
    parser.add_argument(
        "project_id", type=ObjectParser.non_empty_str, required=True)
    parser.add_argument(
        "private_key_id", type=ObjectParser.non_empty_str, required=True)
    parser.add_argument(
        "private_key", type=ObjectParser.non_empty_str, required=True)
    parser.add_argument(
        "client_email", type=ObjectParser.non_empty_str, required=True)
    parser.add_argument(
        "client_id", type=ObjectParser.non_empty_str, required=True)
    parser.add_argument(
        "auth_uri", type=ObjectParser.non_empty_str, required=True)
    parser.add_argument(
        "token_uri", type=ObjectParser.non_empty_str, required=True)
    parser.add_argument(
        "auth_provider_x509_cert_url",
        type=ObjectParser.non_empty_str,
        required=True)
    parser.add_argument(
        "client_x509_cert_url", type=ObjectParser.non_empty_str, required=True)
    parser.add_argument(
        "universe_domain", type=ObjectParser.non_empty_str, required=True)
    return parser

  @classmethod
  def parse_dict(cls, config: Dict) -> ServiceAccount:
    return cls.config_parser().parse(config)

  @classmethod
  def parse_str(cls, value: str):
    del value
    raise NotImplementedError("ServiceAccount from string not supported")

  def to_json(self) -> JsonDict:
    return {
        "type": self.type,
        "project_id": self.project_id,
        "private_key_id": self.private_key_id,
        "private_key": self.private_key,
        "client_email": self.client_email,
        "client_id": self.client_id,
        "auth_uri": self.auth_uri,
        "token_uri": self.token_uri,
        "auth_provider_x509_cert_url": self.auth_provider_x509_cert_url,
        "client_x509_cert_url": self.client_x509_cert_url,
        "universe_domain": self.universe_domain,
    }
