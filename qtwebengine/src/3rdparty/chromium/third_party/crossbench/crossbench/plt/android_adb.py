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

from crossbench import path as pth
from crossbench.parse import NumberParser, PathParser
from crossbench.plt.arch import MachineArch
from crossbench.plt.posix import RemotePosixPlatform

# Defines the Android permissions to be granted.
# TODO(381985595): make this configurable.
ANDROID_PERMISSIONS = ["POST_NOTIFICATIONS", "CAMERA", "RECORD_AUDIO"]

if TYPE_CHECKING:
  from crossbench.plt.base import CmdArg, ListCmdArgs, Platform
  from crossbench.types import JsonDict


def _find_adb_bin(platform: Platform) -> pth.AnyPath:
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
    adb_bin: Optional[pth.AnyPath] = None) -> Dict[str, Dict[str, str]]:
  adb_bin = adb_bin or _find_adb_bin(platform)
  output = platform.sh_stdout(adb_bin, "devices", "-l")
  raw_lines = output.strip().splitlines()[1:]
  result: Dict[str, Dict[str, str]] = {}
  for line in raw_lines:
    serial_id, details = line.split(" ", maxsplit=1)
    result[serial_id.strip()] = _parse_adb_device_info(details.strip())
  return result


def _parse_adb_device_info(value: str) -> Dict[str, str]:
  """
  Convert a line from adb devices -l into a descriptive dictionary.
  `value` is a line of output, typically:
  ABCDEF01234567 device 2-1 product:shiba model:AOSP device:shiba transport_id:3

  Some older versions of adb would not contain the `2-1` part.
  """
  parts = value.split(" ")
  assert parts[0], "device"
  return dict(part.split(":") for part in parts[1:] if ":" in part)


class Adb:

  _serial_id: str
  _device_info: Dict[str, str]
  _adb_bin: pth.AnyPath

  def __init__(self,
               host_platform: Platform,
               device_identifier: Optional[str] = None,
               adb_bin: Optional[pth.AnyPath] = None) -> None:
    self._host_platform = host_platform
    if adb_bin:
      self._adb_bin = PathParser.binary_path(adb_bin, platform=host_platform)
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

  def path(self, path: pth.AnyPathLike) -> pth.AnyPath:
    return pth.AnyPosixPath(path)

  @property
  def serial_id(self) -> str:
    return self._serial_id

  @functools.cached_property
  def build_version(self) -> int:
    return int(self.getprop("ro.build.version.release"))

  @property
  def device_info(self) -> Dict[str, str]:
    return self._device_info

  def build_adb_cmd(self,
                     *args: CmdArg,
                     use_serial_id: bool = True) -> ListCmdArgs:
    adb_cmd: ListCmdArgs = [self._adb_bin]
    if use_serial_id:
      adb_cmd.extend(("-s", self._serial_id))
    adb_cmd.extend(args)
    return adb_cmd

  def _build_shell_args(self,
                        *args: CmdArg,
                        shell: bool = False) -> ListCmdArgs:
    if shell and len(args) != 1:
      raise ValueError("Expected single sh arg with shell=True, "
                       f"but got: {args}")
    return [shlex.join(map(str, args))]

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
    adb_cmd = self.build_adb_cmd(*args, use_serial_id=use_serial_id)
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
                  stdin=None,
                  encoding: str = "utf-8",
                  use_serial_id: bool = True,
                  check: bool = True) -> str:
    result = self._adb_stdout_bytes(
        *args,
        quiet=quiet,
        stdin=stdin,
        use_serial_id=use_serial_id,
        check=check)
    return result.decode(encoding)

  def _adb_stdout_bytes(self,
                        *args: CmdArg,
                        quiet: bool = False,
                        stdin=None,
                        use_serial_id: bool = True,
                        check: bool = True) -> bytes:
    adb_cmd = self.build_adb_cmd(*args, use_serial_id=use_serial_id)
    return self._host_platform.sh_stdout_bytes(
        *adb_cmd, quiet=quiet, check=check, stdin=stdin)

  def shell_stdout(self,
                   *args: CmdArg,
                   shell: bool = False,
                   quiet: bool = False,
                   encoding: str = "utf-8",
                   stdin=None,
                   env: Optional[Mapping[str, str]] = None,
                   check: bool = True) -> str:
    result = self.shell_stdout_bytes(
        *args, shell=shell, quiet=quiet, stdin=stdin, env=env, check=check)
    return result.decode(encoding)

  def shell_stdout_bytes(self,
                         *args: CmdArg,
                         shell: bool = False,
                         quiet: bool = False,
                         stdin=None,
                         env: Optional[Mapping[str, str]] = None,
                         check: bool = True) -> bytes:
    # -e: choose escape character, or "none"; default '~'
    # -n: don't read from stdin
    # -T: disable pty allocation
    # -t: allocate a pty if on a tty (-tt: force pty allocation)
    # -x: disable remote exit codes and stdout/stderr separation
    if env:
      raise ValueError("ADB shell only supports an empty env for now.")
    shell_args = self._build_shell_args(*args, shell=shell)
    return self._adb_stdout_bytes(
        "shell", *shell_args, stdin=stdin, quiet=quiet, check=check)

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
    if env:
      raise ValueError("ADB shell only supports an empty env for now.")
    # See shell_stdout for more `adb shell` options.
    shell_args = self._build_shell_args(*args, shell=shell)
    return self._adb(
        "shell",
        *shell_args,
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

  def root(self) -> None:
    self._adb("root", use_serial_id=False)

  def unroot(self) -> None:
    self._adb("unroot", use_serial_id=False)

  def devices(self) -> Dict[str, Dict[str, str]]:
    return adb_devices(self._host_platform, self._adb_bin)

  def forward(self, local: int, remote: int, protocol: str = "tcp") -> int:
    stdout = self._adb_stdout(
        "forward", f"{protocol}:{local}", f"{protocol}:{remote}")
    local_port = NumberParser.port_number(stdout, "local_port")
    return local_port

  def forward_remove(self, local: int, protocol: str = "tcp") -> None:
    self._adb("forward", "--remove", f"{protocol}:{local}")

  def reverse(self, remote: int, local: int, protocol: str = "tcp") -> int:
    stdout = self._adb_stdout(
        "reverse", f"{protocol}:{remote}", f"{protocol}:{local}")
    remote_port = NumberParser.port_number(stdout, "remote_port")
    return remote_port

  def reverse_remove(self, remote: int, protocol: str = "tcp") -> None:
    self._adb("reverse", "--remove", f"{protocol}:{remote}")

  def pull(self, device_src_path: pth.AnyPath,
           local_dest_path: pth.LocalPath) -> None:
    self._adb("pull", self.path(device_src_path), local_dest_path)

  def push(self, local_src_path: pth.LocalPath,
           device_dest_path: pth.AnyPath) -> None:
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

  def force_clear(self, package_name: str) -> None:
    if not package_name:
      raise ValueError("Got empty package name")
    cmd: ListCmdArgs = ["pm", "clear"]
    if self.build_version >= 14:
      user = self.cmd("user", "get-main-user").strip()
      cmd.extend(["--user", user])
    cmd.extend([package_name])
    self.shell(*cmd)

  def install(self,
              bundle: pth.LocalPath,
              allow_downgrade: bool = False,
              modules: Optional[str] = None) -> None:
    if bundle.suffix == ".apks":
      self.install_apks(bundle, allow_downgrade, modules)
    if bundle.suffix == ".apk":
      self.install_apk(bundle, allow_downgrade)

  def install_apk(self,
                  apk: pth.LocalPath,
                  allow_downgrade: bool = False) -> None:
    if not apk.exists():
      raise ValueError(f"APK {apk} does not exist.")
    args = ["install"]
    if allow_downgrade:
      args.append("-d")
    args.append(str(apk))
    self._adb(*args)

  def install_apks(self,
                   apks: pth.LocalPath,
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

  def grant_permissions(self, package_name: str) -> None:
    if self.build_version < 13:
      # Notification permission setting is needed for Android 13 and above.
      # https://developer.android.com/develop/ui/views/notifications/notification-permission  # pylint: disable=line-too-long
      return
    if not package_name:
      raise ValueError("Got empty package name")
    user: Optional[str] = None
    if self.build_version >= 14:
      user = self.cmd("user", "get-main-user").strip()
    for perm in ANDROID_PERMISSIONS:
      cmd: ListCmdArgs = ["pm", "grant"]
      if user:
        cmd.extend(["--user", user])
      cmd.extend([package_name, f"android.permission.{perm}"])
      self.shell(*cmd)


class AndroidAdbPlatform(RemotePosixPlatform):

  def __init__(self,
               host_platform: Platform,
               device_identifier: Optional[str] = None,
               adb: Optional[Adb] = None) -> None:
    super().__init__(host_platform)
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

  @property
  def serial_id(self):
    return self._adb.serial_id

  @functools.cached_property
  def cpu(self) -> str:  #pylint: disable=invalid-overridden-method
    variant = self.adb.getprop("dalvik.vm.isa.arm.variant")
    platform = self.adb.getprop("ro.board.platform")
    cpu_str = f"{variant} {platform}"
    if num_cores := self.cpu_cores:
      cpu_str = f"{cpu_str} {num_cores} cores"
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

  def get_relative_cpu_speed(self) -> float:
    # TODO figure out
    return 1.0

  @functools.lru_cache(maxsize=1)
  def python_details(self) -> JsonDict:
    # Python is not available on android.
    return {}

  @functools.lru_cache(maxsize=1)
  def os_details(self) -> JsonDict:
    # TODO: add more info
    return {"version": self.version}

  def app_path_to_package(self, app_path: pth.AnyPathLike) -> str:
    path = self.path(app_path)
    parts = path.parts
    if len(parts) > 1:
      raise ValueError(f"Invalid android package name: '{path}'")
    package: str = parts[0]
    packages = self.adb.packages()
    if package not in packages:
      raise ValueError(f"Package '{package}' is not installed on {self._adb}")
    return package

  def search_binary(self, app_or_bin: pth.AnyPathLike) -> Optional[pth.AnyPath]:
    app_or_bin_path = self.path(app_or_bin)
    if not app_or_bin_path.parts:
      raise ValueError("Got empty path")
    if result_path := self.which(app_or_bin_path):
      return result_path
    if str(app_or_bin) in self.adb.packages():
      return app_or_bin_path
    return None

  def home(self) -> pth.AnyPath:
    raise RuntimeError("Cannot access home dir on (non-rooted) android device")

  _VERSION_NAME_RE = re.compile(r"versionName=(?P<version>.+)")

  def app_version(self, app_or_bin: pth.AnyPathLike) -> str:
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
  def default_tmp_dir(self) -> pth.AnyPath:
    return self.path("/data/local/tmp/")

  def build_shell_cmd(self, *args: CmdArg) -> ListCmdArgs:
    return self.adb.build_adb_cmd("shell", *args)

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

  def sh_stdout_bytes(self,
                      *args: CmdArg,
                      shell: bool = False,
                      quiet: bool = False,
                      stdin=None,
                      env: Optional[Mapping[str, str]] = None,
                      check: bool = True) -> bytes:
    return self.adb.shell_stdout_bytes(
        *args, shell=shell, stdin=stdin, env=env, quiet=quiet, check=check)

  def port_forward(self, local_port: int, remote_port: int) -> int:
    local_port = NumberParser.positive_zero_int(local_port, "local_port")
    remote_port = NumberParser.port_number(remote_port, "remote_port")
    local_port = self.adb.forward(local_port, remote_port, protocol="tcp")
    logging.debug("Forwarded Remote Port: %s:%s <= %s:%s",
                  self._host_platform.name, local_port, self, remote_port)
    return local_port

  def stop_port_forward(self, local_port: int) -> None:
    self.adb.forward_remove(local_port, protocol="tcp")

  def reverse_port_forward(self, remote_port: int, local_port: int) -> int:
    remote_port = NumberParser.positive_zero_int(remote_port, "remote_port")
    local_port = NumberParser.port_number(local_port, "local_port")
    remote_port = self.adb.reverse(remote_port, local_port, protocol="tcp")
    logging.debug("Forwarded Local Port: %s:%s => %s:%s", self._host_platform,
                  local_port, self, remote_port)
    return remote_port

  def stop_reverse_port_forward(self, remote_port: int) -> None:
    self.adb.reverse_remove(remote_port, protocol="tcp")

  def pull(self, from_path: pth.AnyPath,
           to_path: pth.LocalPath) -> pth.LocalPath:
    device_path = self.path(from_path)
    if not self.exists(device_path):
      raise ValueError(f"Source file '{from_path}' does not exist on {self}")
    local_host_path = self.host_path(to_path)
    local_host_path.parent.mkdir(parents=True, exist_ok=True)
    self.adb.pull(device_path, local_host_path)
    return to_path

  def push(self, from_path: pth.LocalPath, to_path: pth.AnyPath) -> pth.AnyPath:
    to_path = self.path(to_path)
    self.adb.push(self.host_path(from_path), to_path)
    return to_path

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

  @functools.lru_cache(maxsize=1)
  def cpu_details(self) -> Dict[str, Any]:
    # TODO: Implement properly (i.e. remove all n/a values)
    return {
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

  @functools.lru_cache(maxsize=1)
  def system_details(self) -> Dict[str, Any]:
    # TODO: Implement properly (i.e. remove all n/a values)
    return {
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

  def screenshot(self, result_path: pth.AnyPath) -> None:
    self.sh("screencap", "-p", result_path)

  _WM_SIZE_RE = re.compile(r"Physical size: (?P<x>\d+)x(?P<y>\d+)")

  def display_resolution(self) -> Tuple[int, int]:
    wm_size_out = self.sh_stdout("wm", "size")
    match_result = self._WM_SIZE_RE.match(wm_size_out)
    if match_result is None:
      raise ValueError(f"Could not find display resolution in '{wm_size_out}'")
    x = NumberParser.positive_int(match_result.group("x"))
    y = NumberParser.positive_int(match_result.group("y"))
    return (x, y)
