# Copyright 2025 The Chromium Authors
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

from __future__ import annotations

from typing import TYPE_CHECKING, Optional

from crossbench.action_runner.action.enums import ReadyState
from crossbench.action_runner.action.get import GetAction
from crossbench.action_runner.bond_base import BondActionRunner
from crossbench.bond.bond import BondClient

if TYPE_CHECKING:
  from crossbench.action_runner.action import all as i_action
  from crossbench.action_runner.base import ActionRunner
  from crossbench.runner.run import Run


class DefaultBondActionRunner(BondActionRunner):

  def __init__(self, action_runner: ActionRunner):
    self._action_runner: ActionRunner = action_runner
    self._bond_client: Optional[BondClient] = None

  def bond_client(self, run: Run) -> BondClient:
    if not self._bond_client:
      secret = run.secrets.bond
      if not secret:
        raise RuntimeError("No bond service account secret provided")
      self._bond_client = BondClient(secret)
    return self._bond_client

  def teardown(self):
    if self._bond_client:
      self._bond_client.teardown()
      self._bond_client = None

  def meet_create(self, run: Run, action: i_action.MeetCreateAction):
    bond_client = self.bond_client(run)
    conference_code = bond_client.create_meeting()
    if action.bots:
      bond_client.add_bots(conference_code, action.bots)
    url = f"https://meet.google.com/{conference_code}"
    self._action_runner.get(
        run,
        GetAction(url, ready_state=ReadyState.COMPLETE, target=action.target))
