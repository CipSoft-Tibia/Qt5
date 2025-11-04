# Copyright 2024 The Chromium Authors
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

from __future__ import annotations

import abc
import dataclasses
import enum
import traceback
from typing import TYPE_CHECKING, Any, Dict, Optional, Tuple, Type, TypeVar

from crossbench import exception
from crossbench.config import ConfigEnum, ConfigObject, ConfigParser
from crossbench.runner.actions import Actions
from crossbench.runner.run import Run


@enum.unique
class SecretType(ConfigEnum):
  GOOGLE: "SecretType" = ("google", "Google account name and password")


SecretT = TypeVar("SecretT", bound="Secret")


@dataclasses.dataclass(frozen=True)
class SecretsConfig(ConfigObject):
  secrets: Dict[SecretType, Secret]

  @classmethod
  def parse_str(cls, value: str) -> SecretsConfig:
    raise NotImplementedError("Cannot create secrets from string")

  @classmethod
  def parse_dict(cls, config: Dict) -> SecretsConfig:
    secrets = {}
    for type_str, c in config.items():
      secret_type = SecretType.parse(type_str)
      secret_cls: Type = SECRETS[secret_type]

      with exception.annotate_argparsing("Parsing Secret details:"):
        secret = secret_cls.config_parser().parse(c)

      assert isinstance(secret,
                        Secret), f"Expected {cls} but got {type(secret)}"
      assert secret_type not in secrets, f"Duplicate entry for {type_str}"

      secrets[secret_type] = secret
    return SecretsConfig(secrets)


class Secret(metaclass=abc.ABCMeta):

  @abc.abstractmethod
  def login(self, run: Run) -> None:
    pass


class GoogleSecret(Secret):

  @classmethod
  def config_parser(cls: Type[SecretT]) -> ConfigParser[SecretT]:
    parser = ConfigParser(f"{cls.__name__} parser", cls)
    parser.add_argument("account", type=str, required=True)
    parser.add_argument("password", type=str, required=True)
    return parser

  def __init__(self, account: str, password: str) -> None:
    self._account: str = account
    self._password = password

  def _submit_login_field(self, action: Actions, aria_label: str,
                          input_val: str, button_name: str) -> None:
    action.wait_js_condition(
        f"""
        return document.querySelector("[aria-label='{aria_label}']") != null
          && document.querySelector("[id={button_name}]") != null;
        """, .2, 10)
    action.js(f"""
        var inputField = document.querySelector("[aria-label='{aria_label}']");
        inputField.value = '{input_val}';
        document.querySelector("[id={button_name}]").click();
       """)

  def login(self, run: Run) -> None:
    url = "https://accounts.google.com/Logout?continue=https%3A%2F%2Faccounts.google.com%2Fv3%2Fsignin%2Fidentifier%3FflowName%3DGlifWebSignIn%26flowEntry%3DServiceLogin"
    with run.actions("Login") as action:
      action.show_url(url)
      self._submit_login_field(action, "Email or phone", self._account,
                               "identifierNext")
      self._submit_login_field(action, "Enter your password", self._password,
                               "passwordNext")
      action.wait_js_condition(
          """return document.URL.startsWith('https://myaccount.google.com'); """,
          .2, 10)


SECRETS: Dict[SecretType, Type] = {
    SecretType.GOOGLE: GoogleSecret,
}
