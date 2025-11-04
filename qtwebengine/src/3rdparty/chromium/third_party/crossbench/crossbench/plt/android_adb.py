# Copyright 2023 The Chromium Authors
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

from __future__ import annotations

import functools
import logging
import re
import shlex
import subprocess
from typing import TYPE_CHECKING, Any, Dict, List, Mapping, Optional, Tuple

from crossbench import cli_helper
from crossbench.plt.arch import MachineArch
from crossbench.plt.posix import RemotePosixPlatform

if TYPE_CHECKING:
  from crossbench.path import (LocalPath, LocalPathLike, RemotePath,
                               RemotePathLike)
  from crossbench.plt.base import CmdArg, ListCmdArgs, Platform
  from crossbench.types import JsonDict


def _find_adb_bin(platform: Platform) -> RemotePath:
  adb_bin = platform.search_platform_binary(
      name="adb",
      macos=["adb", "~/Library/Android/sdk/platform-tools/adb"],
      linux=["adb"],
      win=["adb.exe", "Android/sdk/platform-tools/adb.exe"])
  if adb_bin:
    return adb_bin
  raise ValueError(
      "Could not find adb binary."
      "See https://developer.android.com/tools/adb fore more details.")


def adb_devices(
    platform: Platform,
    adb_bin: Optional[RemotePath] = None) -> Dict[str, Dict[str, str]]:
  adb_bin = adb_bin or _find_adb_bin(platform)
  output = platform.sh_stdout(adb_bin, "devices", "-l")
  raw_lines = output.strip().splitlines()[1:]
  result: Dict[str, Dict[str, str]] = {}
  for line in raw_lines:
    serial_id, details = line.split(" ", maxsplit=1)
    result[serial_id.strip()] = _parse_adb_device_info(details.strip())
  return result


def _parse_adb_device_info(value: str) -> Dict[str, str]:
  parts = value.split(" ")
  assert parts[0], "device"
  return dict(part.split(":") for part in parts[1:])


class Adb:

  _serial_id: str
  _device_info: Dict[str, str]
  _adb_bin: RemotePath

  def __init__(self,
               host_platform: Platform,
               device_identifier: Optional[str] = None,
               adb_bin: Optional[RemotePath] = None) -> None:
    self._host_platform = host_platform
    if adb_bin:
      self._adb_bin = cli_helper.parse_binary_path(
          adb_bin, platform=host_platform)
    else:
      self._adb_bin = _find_adb_bin(host_platform)
    self.start_server()
    self._serial_id, self._device_info = self._find_serial_id(device_identifier)
    logging.debug("ADB Selected device: %s %s", self._serial_id,
                  self._device_info)
    assert self._serial_id

  def _find_serial_id(
      self,
      device_identifier: Optional[str] = None) -> Tuple[str, Dict[str, str]]:
    devices = self.devices()
    if not devices:
      raise ValueError("adb could not find any attached devices."
                       "Connect your device and use 'adb devices' to list all.")
    if device_identifier is None:
      if len(devices) != 1:
        raise ValueError(
            f"Too many adb devices attached, please specify one of: {devices}")
      device_identifier = list(devices.keys())[0]
    if not device_identifier:
      raise ValueError(f"Invalid device identifier: {repr(device_identifier)}")
    if device_identifier in devices:
      return device_identifier, devices[device_identifier]
    matches: List[str] = []
    under_name = device_identifier.replace(" ", "_")
    for key, device_info in devices.items():
      for _, info_value in device_info.items():
        if device_identifier in info_value or (under_name in info_value):
          matches.append(key)
    if not matches:
      raise ValueError(
          f"Could not find adb device matching: '{device_identifier}'")
    if len(matches) > 1:
      raise ValueError(
          f"Found {len(matches)} adb devices matching: '{device_identifier}'.\n"
          f"Choices: {matches}")
    return matches[0], devices[matches[0]]

  def __str__(self) -> str:
    info = f"info='{self._device_info}'"
    if model := self._device_info.get("model"):
      info = f"model={repr(model)}"
    return f"adb(device_id={repr(self._serial_id)}, {info})"

  def has_root(self) -> bool:
    return self.shell_stdout("id").startswith("uid=0(root)")

  def path(self, path: RemotePathLike) -> RemotePath:
    return self._host_platform.path(path)

  @property
  def serial_id(self) -> str:
    return self._serial_id

  @functools.cached_property
  def build_version(self) -> int:
    return int(self.getprop("ro.build.version.release"))

  @property
  def device_info(self) -> Dict[str, str]:
    return self._device_info

  def popen(self,
            *args: CmdArg,
            shell: bool = False,
            stdout=None,
            stderr=None,
            stdin=None,
            env: Optional[Mapping[str, str]] = None,
            quiet: bool = False) -> subprocess.Popen:
    del shell
    assert not env, "ADB does not support setting env vars."
    if not quiet:
      logging.debug("SHELL: %s", shlex.join(map(str, args)))
    adb_cmd: ListCmdArgs = [self._adb_bin, "-s", self._serial_id, "shell"]
    adb_cmd.extend(args)
    return self._host_platform.popen(
        *adb_cmd, stdout=stdout, stderr=stderr, stdin=stdin)

  def _adb(self,
           *args: CmdArg,
           shell: bool = False,
           capture_output: bool = False,
           stdout=None,
           stderr=None,
           stdin=None,
           env: Optional[Mapping[str, str]] = None,
           quiet: bool = False,
           check: bool = True,
           use_serial_id: bool = True) -> subprocess.CompletedProcess:
    del shell
    adb_cmd: ListCmdArgs = []
    if use_serial_id:
      adb_cmd = [self._adb_bin, "-s", self._serial_id]
    else:
      adb_cmd = [self._adb_bin]
    adb_cmd.extend(args)
    return self._host_platform.sh(
        *adb_cmd,
        capture_output=capture_output,
        stdout=stdout,
        stderr=stderr,
        stdin=stdin,
        env=env,
        quiet=quiet,
        check=check)

  def _adb_stdout(self,
                  *args: CmdArg,
                  quiet: bool = False,
                  encoding: str = "utf-8",
                  use_serial_id: bool = True,
                  check: bool = True) -> str:
    adb_cmd: ListCmdArgs = []
    if use_serial_id:
      adb_cmd = [self._adb_bin, "-s", self._serial_id]
    else:
      adb_cmd = [self._adb_bin]
    adb_cmd.extend(args)
    return self._host_platform.sh_stdout(
        *adb_cmd, quiet=quiet, encoding=encoding, check=check)

  def shell_stdout(self,
                   *args: CmdArg,
                   quiet: bool = False,
                   encoding: str = "utf-8",
                   env: Optional[Mapping[str, str]] = None,
                   check: bool = True) -> str:
    # -e: choose escape character, or "none"; default '~'
    # -n: don't read from stdin
    # -T: disable pty allocation
    # -t: allocate a pty if on a tty (-tt: force pty allocation)
    # -x: disable remote exit codes and stdout/stderr separation
    if env:
      raise ValueError("ADB shell only supports an empty env for now.")
    # Need to escape spaces in args for adb shell
    args = map(lambda x: str(x).replace(" ", "\\ "), args)
    return self._adb_stdout(
        "shell", *args, quiet=quiet, encoding=encoding, check=check)

  def shell(self,
            *args: CmdArg,
            shell: bool = False,
            capture_output: bool = False,
            stdout=None,
            stderr=None,
            stdin=None,
            env: Optional[Mapping[str, str]] = None,
            quiet: bool = False,
            check: bool = True) -> subprocess.CompletedProcess:
    # See shell_stdout for more `adb shell` options.
    adb_cmd: ListCmdArgs = ["shell", *args]
    return self._adb(
        *adb_cmd,
        shell=shell,
        capture_output=capture_output,
        stdout=stdout,
        stderr=stderr,
        stdin=stdin,
        env=env,
        quiet=quiet,
        check=check)

  def start_server(self) -> None:
    self._adb_stdout("start-server", use_serial_id=False)

  def stop_server(self) -> None:
    self.kill_server()

  def kill_server(self) -> None:
    self._adb_stdout("kill-server", use_serial_id=False)

  def devices(self) -> Dict[str, Dict[str, str]]:
    return adb_devices(self._host_platform, self._adb_bin)

  def reverse(self, remote: int, local: int, protocol: str = "tcp") -> None:
    self._adb("reverse", f"{protocol}:{remote}", f"{protocol}:{local}")

  def reverse_remove(self, remote: int, protocol: str = "tcp") -> None:
    self._adb("reverse", "--remove", f"{protocol}:{remote}")

  def pull(self, device_src_path: RemotePath,
           local_dest_path: LocalPath) -> None:
    self._adb("pull", self.path(device_src_path), local_dest_path)

  def push(self, local_src_path: LocalPath,
           device_dest_path: RemotePath) -> None:
    self._adb("push", local_src_path, self.path(device_dest_path))

  def cmd(self,
          *args: str,
          quiet: bool = False,
          encoding: str = "utf-8") -> str:
    cmd: ListCmdArgs = ["cmd", *args]
    return self.shell_stdout(*cmd, quiet=quiet, encoding=encoding)

  def dumpsys(self,
              *args: str,
              quiet: bool = False,
              encoding: str = "utf-8") -> str:
    cmd: ListCmdArgs = ["dumpsys", *args]
    return self.shell_stdout(*cmd, quiet=quiet, encoding=encoding)

  def getprop(self,
              *args: str,
              quiet: bool = False,
              encoding: str = "utf-8") -> str:
    cmd: ListCmdArgs = ["getprop", *args]
    return self.shell_stdout(*cmd, quiet=quiet, encoding=encoding).strip()

  def services(self, quiet: bool = False, encoding: str = "utf-8") -> List[str]:
    lines = list(
        self.cmd("-l", quiet=quiet, encoding=encoding).strip().splitlines())
    lines = lines[1:]
    lines.sort()
    return [line.strip() for line in lines]

  def packages(self, quiet: bool = False, encoding: str = "utf-8") -> List[str]:
    # adb shell cmd package list packages
    raw_list = self.cmd(
        "package", "list", "packages", quiet=quiet,
        encoding=encoding).strip().splitlines()
    packages = [package.split(":", maxsplit=2)[1] for package in raw_list]
    packages.sort()
    return packages

  def force_stop(self, package_name: str) -> None:
    if not package_name:
      raise ValueError("Got empty package name")
    self.shell("am", "force-stop", package_name)

  def install(self,
              bundle: LocalPath,
              allow_downgrade: bool = False,
              modules: Optional[str] = None) -> None:
    if bundle.suffix == ".apks":
      self.install_apks(bundle, allow_downgrade, modules)
    if bundle.suffix == ".apk":
      self.install_apk(bundle, allow_downgrade)

  def install_apk(self, apk: LocalPath, allow_downgrade: bool = False) -> None:
    if not apk.exists():
      raise ValueError(f"APK {apk} does not exist.")
    args = ["install"]
    if allow_downgrade:
      args.append("-d")
    args.append(str(apk))
    self._adb(*args)

  def install_apks(self,
                   apks: LocalPath,
                   allow_downgrade: bool = False,
                   modules: Optional[str] = None) -> None:
    if not apks.exists():
      raise ValueError(f"APK {apks} does not exist.")
    cmd = [
        "bundletool",
        "install-apks",
        f"--apks={apks}",
        f"--device-id={self._serial_id}",
    ]
    if allow_downgrade:
      cmd.append("--allow-downgrade")
    if modules:
      cmd.append(f"--modules={modules}")
    self._host_platform.sh(*cmd)

  def uninstall(self, package_name: str, missing_ok: bool = False) -> None:
    if not package_name:
      raise ValueError("Got empty package name")
    try:
      self._adb("uninstall", package_name)
    except Exception as e:  # pylint: disable=broad-except
      if missing_ok:
        logging.debug("Could not uninstall %s: %s", package_name, e)
      else:
        raise

  def grant_notification_permissions(self, package_name: str) -> None:
    if self.build_version < 13:
      # Notification permission setting is needed for Android 13 and above.
      # https://developer.android.com/develop/ui/views/notifications/notification-permission  # pylint: disable=line-too-long
      return
    if not package_name:
      raise ValueError("Got empty package name")
    cmd: ListCmdArgs = ["pm", "grant"]
    if self.build_version >= 14:
      user = self.cmd("user", "get-main-user").strip()
      cmd.extend(["--user", user])
    cmd.extend([package_name, "android.permission.POST_NOTIFICATIONS"])
    self.shell(*cmd)


class AndroidAdbPlatform(RemotePosixPlatform):

  def __init__(self,
               host_platform: Platform,
               device_identifier: Optional[str] = None,
               adb: Optional[Adb] = None) -> None:
    super().__init__(host_platform)
    self._system_details: Optional[Dict[str, Any]] = None
    self._cpu_details: Optional[Dict[str, Any]] = None
    assert not host_platform.is_remote, (
        "adb on remote platform is not supported yet")
    self._adb = adb or Adb(host_platform, device_identifier)

  @property
  def is_android(self) -> bool:
    return True

  @property
  def name(self) -> str:
    return "android"

  @functools.cached_property
  def version(self) -> str:  #pylint: disable=invalid-overridden-method
    return str(self.adb.build_version)

  @functools.cached_property
  def device(self) -> str:  #pylint: disable=invalid-overridden-method
    return self.adb.getprop("ro.product.model")

  @functools.cached_property
  def cpu(self) -> str:  #pylint: disable=invalid-overridden-method
    variant = self.adb.getprop("dalvik.vm.isa.arm.variant")
    platform = self.adb.getprop("ro.board.platform")
    cpu_str = f"{variant} {platform}"
    if cores_info := self._get_cpu_cores_info():
      cpu_str = f"{cpu_str} {cores_info}"
    return cpu_str

  @property
  def adb(self) -> Adb:
    return self._adb

  _MACHINE_ARCH_LOOKUP = {
      "arm64-v8a": MachineArch.ARM_64,
      "armeabi-v7a": MachineArch.ARM_32,
      "x86": MachineArch.IA32,
      "x86_64": MachineArch.X64,
  }

  @functools.cached_property
  def machine(self) -> MachineArch:  #pylint: disable=invalid-overridden-method
    cpu_abi = self.adb.getprop("ro.product.cpu.abi")
    arch = self._MACHINE_ARCH_LOOKUP.get(cpu_abi, None)
    if not arch:
      raise ValueError(f"Unknown android CPU ABI: {cpu_abi}")
    return arch

  def app_path_to_package(self, app_path: RemotePathLike) -> str:
    path = self.path(app_path)
    if len(path.parts) > 1:
      raise ValueError(f"Invalid android package name: '{path}'")
    package: str = path.parts[0]
    packages = self.adb.packages()
    if package not in packages:
      raise ValueError(f"Package '{package}' is not installed on {self._adb}")
    return package

  def search_binary(self, app_or_bin: RemotePathLike) -> Optional[RemotePath]:
    app_or_bin_path = self.path(app_or_bin)
    if not app_or_bin_path.parts:
      raise ValueError("Got empty path")
    if result_path := self.which(app_or_bin_path):
      return result_path
    if str(app_or_bin) in self.adb.packages():
      return app_or_bin_path
    return None

  def home(self) -> RemotePath:
    raise RuntimeError("Cannot access home dir on (non-rooted) android device")

  _VERSION_NAME_RE = re.compile(r"versionName=(?P<version>.+)")

  def app_version(self, app_or_bin: RemotePathLike) -> str:
    # adb shell dumpsys package com.chrome.canary | grep versionName -C2
    package = self.app_path_to_package(app_or_bin)
    package_info = self.adb.dumpsys("package", str(package))
    match_result = self._VERSION_NAME_RE.search(package_info)
    if match_result is None:
      raise ValueError(
          f"Could not find version for '{package}': {package_info}")
    return match_result.group("version")

  def process_children(self,
                       parent_pid: int,
                       recursive: bool = False) -> List[Dict[str, Any]]:
    # TODO: implement
    return []

  def foreground_process(self) -> Optional[Dict[str, Any]]:
    # adb shell dumpsys activity activities
    # TODO: implement
    return None

  def get_relative_cpu_speed(self) -> float:
    # TODO figure out
    return 1.0

  def python_details(self) -> JsonDict:
    # Python is not available on android.
    return {}

  def os_details(self) -> JsonDict:
    # TODO: add more info
    return {"version": self.version}

  def check_autobrightness(self) -> bool:
    # adb shell dumpsys display
    # TODO: implement.
    return True

  _BRIGHTNESS_RE = re.compile(
      r"mLatestFloatBrightness=(?P<brightness>[0-9]+\.[0-9]+)")

  def get_main_display_brightness(self) -> int:
    display_info: str = self.adb.shell_stdout("dumpsys", "display")
    match_result = self._BRIGHTNESS_RE.search(display_info)
    if match_result is None:
      raise ValueError("Could not parse adb display brightness.")
    return int(float(match_result.group("brightness")) * 100)

  @property
  def default_tmp_dir(self) -> RemotePath:
    return self.path("/data/local/tmp/")

  def sh(self,
         *args: CmdArg,
         shell: bool = False,
         capture_output: bool = False,
         stdout=None,
         stderr=None,
         stdin=None,
         env: Optional[Mapping[str, str]] = None,
         quiet: bool = False,
         check: bool = False) -> subprocess.CompletedProcess:
    return self.adb.shell(
        *args,
        shell=shell,
        capture_output=capture_output,
        stdout=stdout,
        stderr=stderr,
        stdin=stdin,
        env=env,
        quiet=quiet,
        check=check)

  def sh_stdout(self,
                *args: CmdArg,
                shell: bool = False,
                quiet: bool = False,
                encoding: str = "utf-8",
                env: Optional[Mapping[str, str]] = None,
                check: bool = True) -> str:
    # The shell option is not supported on adb.
    del shell
    return self.adb.shell_stdout(
        *args, env=env, quiet=quiet, encoding=encoding, check=check)

  def popen(self,
            *args: CmdArg,
            shell: bool = False,
            stdout=None,
            stderr=None,
            stdin=None,
            env: Optional[Mapping[str, str]] = None,
            quiet: bool = False) -> subprocess.Popen:
    return self.adb.popen(
        *args,
        shell=shell,
        stdout=stdout,
        stderr=stderr,
        stdin=stdin,
        env=env,
        quiet=quiet)

  def reverse_port_forward(self, remote_port: int, local_port: int) -> None:
    self.adb.reverse(remote_port, local_port, protocol="tcp")

  def stop_reverse_port_forward(self, remote_port: int) -> None:
    self.adb.reverse_remove(remote_port, protocol="tcp")

  def rsync(self, from_path: RemotePath, to_path: LocalPath) -> LocalPath:
    return self.pull(from_path, to_path)

  def pull(self, from_path: RemotePath, to_path: LocalPath) -> LocalPath:
    device_path = self.path(from_path)
    assert self.exists(device_path), (
        f"Source file '{from_path}' does not exist on {self}")
    local_host_path = self.host_path(to_path)
    local_host_path.parent.mkdir(parents=True, exist_ok=True)
    self.adb.pull(device_path, local_host_path)
    return to_path

  def push(self, from_path: LocalPath, to_path: RemotePath) -> RemotePath:
    self.adb.push(from_path, self.path(to_path))
    return to_path

  def set_file_contents(self,
                        file: RemotePathLike,
                        data: str,
                        encoding: str = "utf-8") -> None:
    # self.push a tmp file with the given contents
    tmp_dir: LocalPath = self.host_path(self.host_platform.mkdtemp())
    try:
      tmp_file = tmp_dir / "push.data"
      with tmp_file.open("w", encoding=encoding) as f:
        f.write(data)
      self.push(tmp_file, self.path(file))
    finally:
      self.host_platform.rm(tmp_dir, dir=True, missing_ok=True)

  def processes(self,
                attrs: Optional[List[str]] = None) -> List[Dict[str, Any]]:
    lines = self.sh_stdout("ps", "-A", "-o", "PID,NAME").splitlines()
    if len(lines) == 1:
      return []

    res: List[Dict[str, Any]] = []
    for line in lines[1:]:
      tokens = line.strip().split(maxsplit=1)
      assert len(tokens) == 2, f"Got invalid process tokens: {tokens}"
      res.append({"pid": int(tokens[0]), "name": tokens[1]})
    return res

  def cpu_details(self) -> Dict[str, Any]:
    if self._cpu_details:
      return self._cpu_details
    # TODO: Implement properly (i.e. remove all n/a values)
    self._cpu_details = {
        "info": self.cpu,
        "physical cores": "n/a",
        "logical cores": "n/a",
        "usage": "n/a",
        "total usage": "n/a",
        "system load": "n/a",
        "max frequency": "n/a",
        "min frequency": "n/a",
        "current frequency": "n/a",
    }
    return self._cpu_details

  _GETPROP_RE = re.compile(r"^\[(?P<key>[^\]]+)\]: \[(?P<value>[^\]]+)\]$")

  def _getprop_system_details(self) -> Dict[str, Any]:
    details = super().system_details()
    properties: Dict[str, str] = {}
    for line in self.adb.shell_stdout("getprop").strip().splitlines():
      result = self._GETPROP_RE.fullmatch(line)
      if result:
        properties[result.group("key")] = result.group("value")
    details["android"] = properties
    return details

  def system_details(self) -> Dict[str, Any]:
    if self._system_details:
      return self._system_details

    # TODO: Implement properly (i.e. remove all n/a values)
    self._system_details = {
        "machine": self.sh_stdout("uname", "-m").split()[0],
        "os": {
            "system": self.sh_stdout("uname", "-s").split()[0],
            "release": self.sh_stdout("uname", "-r").split()[0],
            "version": self.sh_stdout("uname", "-v").split()[0],
            "platform": "n/a",
        },
        "python": {
            "version": "n/a",
            "bits": "n/a",
        },
        "CPU": self.cpu_details(),
        "Android": self._getprop_system_details(),
    }
    return self._system_details

  def screenshot(self, result_path: RemotePath) -> None:
    self.sh("screencap", "-p", result_path)
