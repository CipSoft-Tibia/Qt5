# Copyright 2024 The Chromium Authors
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

from __future__ import annotations

import atexit
import datetime as dt
import logging
import shlex
import subprocess
from typing import TYPE_CHECKING, Any, Dict, List, Optional

from crossbench import parse
from crossbench.plt.arch import MachineArch
from crossbench.plt.linux import RemoteLinuxPlatform
from crossbench.plt.ssh import SshPlatformMixin

if TYPE_CHECKING:
  from crossbench.path import AnyPath, LocalPath
  from crossbench.plt.base import CmdArg, CmdArgs, ListCmdArgs, Platform


class LinuxSshPlatform(SshPlatformMixin, RemoteLinuxPlatform):

  PORT_FORWARDING_TIMEOUT = dt.timedelta(seconds=10)

  def __init__(self, host_platform: Platform, host: str, port: int,
               ssh_port: int, ssh_user: str) -> None:
    super().__init__(host_platform, host, port, ssh_port, ssh_user)
    self._machine: Optional[MachineArch] = None
    self._system_details: Optional[Dict[str, Any]] = None
    self._cpu_details: Optional[Dict[str, Any]] = None
    self._port_forward_popens: Dict[int, subprocess.Popen] = {}
    self._reverse_port_forward_popens: Dict[int, subprocess.Popen] = {}
    atexit.register(self._stop_all_port_forward)

  @property
  def name(self) -> str:
    return "linux_ssh"

  def _build_ssh_cmd(self, *args: CmdArg, shell: bool = False) -> ListCmdArgs:
    self._validate_shell_args(shell, args)
    ssh_cmd: ListCmdArgs = [
        "ssh", "-p", f"{self._ssh_port}", f"{self._ssh_user}@{self._host}"
    ]
    ssh_cmd.append(shlex.join(map(str, args)))

    if shell:
      combined_ssh_cmd: str = ""

      for cmd in ssh_cmd:
        combined_ssh_cmd = combined_ssh_cmd + str(cmd) + " "

      return [combined_ssh_cmd]

    return ssh_cmd

  def build_shell_cmd(self, *args: CmdArg) -> ListCmdArgs:
    return self._build_ssh_cmd(*args)

  def processes(self,
                attrs: Optional[List[str]] = None) -> List[Dict[str, Any]]:
    # TODO: Define a more generic method in PosixPlatform, possibly with
    # an overridable function to generate ps command line.
    lines = self.sh_stdout("ps", "-A", "-o", "pid,cmd").splitlines()
    if len(lines) == 1:
      return []

    res: List[Dict[str, Any]] = []
    for line in lines[1:]:
      pid, name = line.split(maxsplit=1)
      res.append({"pid": int(pid), "name": name})
    return res

  def push(self, from_path: LocalPath, to_path: AnyPath) -> AnyPath:
    self.mkdir(to_path.parent, parents=True, exist_ok=True)

    scp_cmd: CmdArgs = [
        "scp", "-P", f"{self._ssh_port}", f"{from_path}",
        f"{self._ssh_user}@{self._host}:{to_path}"
    ]
    self._host_platform.sh_stdout(*scp_cmd)
    return to_path

  def pull(self, from_path: AnyPath, to_path: LocalPath) -> LocalPath:
    self._host_platform.mkdir(to_path.parent, parents=True, exist_ok=True)

    scp_cmd: CmdArgs = [
        "scp", "-P", f"{self._ssh_port}",
        f"{self._ssh_user}@{self._host}:{from_path}", to_path
    ]
    self._host_platform.sh_stdout(*scp_cmd)
    return to_path

  def port_forward(self, local_port: int, remote_port: int) -> int:
    local_port, remote_port = self._validate_forwarding_ports(
        local_port, remote_port)
    self._port_forward_popens[local_port] = self.host_platform.popen(
        *self._build_ssh_cmd("-NL", f"{local_port}:localhost:{remote_port}"))
    self.host_platform.wait_for_port(local_port, self.PORT_FORWARDING_TIMEOUT)
    logging.debug("Forwarded Remote Port: %s:%s <= %s:%s", self._host_platform,
                  local_port, self, remote_port)
    return local_port

  def _validate_forwarding_ports(self, local_port, remote_port):
    local_port = parse.NumberParser.positive_zero_int(local_port, "local_port")
    remote_port = parse.NumberParser.port_number(remote_port, "remote_port")
    if not local_port:
      local_port = self.host_platform.get_free_port()
    if local_port in self._port_forward_popens:
      raise RuntimeError(f"Cannot forward local port {local_port} twice.")
    return local_port, remote_port

  def stop_port_forward(self, local_port: int) -> None:
    self._port_forward_popens.pop(local_port).terminate()

  def reverse_port_forward(self, remote_port: int, local_port: int) -> int:
    # TODO: this should likely match with adb, where we support 0
    # for auto-allocating a remote_port
    remote_port, local_port = self._validate_reverse_forwarding_ports(
        remote_port, local_port)
    self._reverse_port_forward_popens[remote_port] = self.host_platform.popen(
        *self._build_ssh_cmd("-NR", f"{remote_port}:localhost:{local_port}"))
    self.wait_for_port(remote_port, self.PORT_FORWARDING_TIMEOUT)
    logging.debug("Forwarded Local Port: %s:%s => %s:%s", self._host_platform,
                  local_port, self, remote_port)
    return remote_port

  def _validate_reverse_forwarding_ports(self, remote_port, local_port):
    remote_port = parse.NumberParser.port_number(remote_port, "remote_port")
    local_port = parse.NumberParser.positive_zero_int(local_port, "local_port")
    if not local_port:
      local_port = self.host_platform.get_free_port()
    if remote_port in self._reverse_port_forward_popens:
      raise RuntimeError(f"Cannot forward remote port {remote_port} twice.")
    return remote_port, local_port

  def stop_reverse_port_forward(self, remote_port: int) -> None:
    self._reverse_port_forward_popens.pop(remote_port).terminate()

  def _stop_all_port_forward(self) -> None:
    for port in list(self._port_forward_popens.keys()):
      self.stop_port_forward(port)
    for port in list(self._reverse_port_forward_popens.keys()):
      self.stop_reverse_port_forward(port)

    assert not self._port_forward_popens, (
        "Did not stop all port forwarding processes.")
    assert not self._reverse_port_forward_popens, (
        "Did not stop all reverse port forwarding processes.")
