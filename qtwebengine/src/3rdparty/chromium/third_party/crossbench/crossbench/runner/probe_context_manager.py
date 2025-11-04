# Copyright 2024 The Chromium Authors
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

from __future__ import annotations

import abc
import contextlib
import datetime as dt
import logging
from typing import (TYPE_CHECKING, Generic, Iterable, List, Optional, Tuple,
                    TypeVar)

from crossbench.helper.state import State, StateMachine
from crossbench.probes.probe import Probe
from crossbench.probes.probe_context import BaseProbeContext
from crossbench.probes.results import EmptyProbeResult, ProbeResult
from crossbench.runner.result_origin import ResultOrigin

if TYPE_CHECKING:
  from crossbench.probes.results import ProbeResultDict

ResultOriginT = TypeVar("ResultOriginT", bound=ResultOrigin)
ProbeContextT = TypeVar("ProbeContextT", bound=BaseProbeContext)


class ProbeContextManager(Generic[ResultOriginT, ProbeContextT], abc.ABC):

  def __init__(self, result_origin: ResultOriginT,
               probe_results: ProbeResultDict):
    self._state = StateMachine(State.INITIAL)
    self._origin = result_origin
    self._probe_results = probe_results
    self._probe_contexts: List[ProbeContextT] = []
    # TODO: either prefix timers or use custom duration
    self._durations = result_origin.durations
    self._exceptions = result_origin.exceptions

  @property
  def is_ready(self) -> bool:
    return self._state == State.READY

  @property
  def is_running(self) -> bool:
    return self._state == State.RUN

  def measure(self, name):
    return self._origin.measure(name)

  @contextlib.contextmanager
  def capture(self, label: str, measure: bool = False):
    with self._exceptions.capture(label):
      if not measure:
        yield
      else:
        with self._origin.durations.measure(label):
          yield

  @property
  def is_success(self) -> bool:
    return self._exceptions.is_success

  def setup(self, probes: Iterable[Probe], is_dry_run: bool):
    self._state.transition(State.INITIAL, to=State.SETUP)
    if not is_dry_run:
      if not self._setup_probes(tuple(probes), is_dry_run):
        return
    self._state.transition(State.SETUP, to=State.READY)

  def _setup_probes(self, probes: Tuple[Probe, ...], is_dry_run: bool) -> bool:
    with self.capture("probes-setup", measure=True):
      self._validate_probes(probes)
      self._create_contexts(probes)
      self._setup_contexts()
    if not self.is_success:
      self._handle_setup_error(is_dry_run)
    return self.is_success

  def _validate_probes(self, probes: Tuple[Probe, ...]):
    assert not self._probe_contexts, "Wrong probe context initialization order"
    probe_set = set()
    for probe in probes:
      assert probe not in probe_set, (f"Got duplicate probe name={probe.name}")
      probe_set.add(probe)
      assert probe.is_attached, (
          f"Probe {probe.name} is not properly attached to a browser")

  def _create_contexts(self, probes: Tuple[Probe, ...]):
    for probe in probes:
      if probe.PRODUCES_DATA:
        self._probe_results[probe] = EmptyProbeResult()
      with self.capture(f"{probe.name} get_context"):
        if probe_context := self.get_probe_context(probe):
          self._probe_contexts.append(probe_context)

  def _setup_contexts(self):
    for probe_context in self._probe_contexts:
      with self.capture(f"probes-setup {probe_context.name}"):
        probe_context.setup()  # pytype: disable=wrong-arg-types

  def _handle_setup_error(self, is_dry_run: bool) -> None:
    self._state.transition(State.SETUP, to=State.DONE)
    logging.debug("Handling setup error")
    assert not self.is_success
    # Special handling for crucial runner probes
    internal_probe_contexts = [
        context for context in self._probe_contexts if context.probe.is_internal
    ]
    self._teardown(internal_probe_contexts, is_dry_run, setup_error=True)

  @contextlib.contextmanager
  def open(self, is_dry_run: bool):
    self._state.transition(State.READY, to=State.RUN)
    probe_start_time = dt.datetime.now()
    combined_contexts = contextlib.ExitStack()

    for probe_context in self._probe_contexts:
      probe_context.set_start_time(probe_start_time)
      if not is_dry_run:
        combined_contexts.enter_context(probe_context.open())

    with combined_contexts:
      self._durations["probes-start"] = dt.datetime.now() - probe_start_time
      yield self

  def teardown(self, is_dry_run: bool, setup_error: bool = False) -> None:
    self._state.transition(State.READY, State.RUN, to=State.DONE)
    with self.measure("probes-teardown"):
      self._teardown(self._probe_contexts, is_dry_run, setup_error)
      self._probe_contexts = []

  def _teardown(self,
                probe_contexts: List[ProbeContextT],
                is_dry_run: bool,
                setup_error: bool = False) -> None:
    if setup_error:
      assert self._probe_contexts, "Invalid state"
    self._state.expect(State.DONE)
    logging.debug("PROBE SCOPE TEARDOWN")
    if is_dry_run:
      return
    for probe_context in reversed(probe_contexts):
      with self.capture(f"Probe {probe_context.name} teardown", measure=True):
        assert probe_context.result_origin == self._origin
        probe_results: ProbeResult = probe_context.teardown()
        probe = probe_context.probe
        if probe_results.is_empty:
          logging.warning("Probe did not extract any data. probe=%s in %s",
                          probe, self._origin)
        self._probe_results[probe] = probe_results

  @abc.abstractmethod
  def get_probe_context(self, probe: Probe) -> Optional[ProbeContextT]:
    pass
