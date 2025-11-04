# Copyright 2023 The Chromium Authors
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

from __future__ import annotations

import abc
import collections.abc
import contextlib
import datetime as dt
import functools
import logging
import os
import pathlib
import platform as py_platform
import shlex
import shutil
import subprocess
import sys
import tempfile
import time
import urllib.error
import urllib.parse
import urllib.request
from typing import (TYPE_CHECKING, Any, Callable, Dict, Generator, Iterable,
                    Iterator, List, Mapping, Optional, Sequence, Tuple, Union)

import psutil

from crossbench import path as pth
from crossbench.plt.arch import MachineArch
from crossbench.plt.bin import Binary

if TYPE_CHECKING:
  from crossbench.types import JsonDict


CmdArg = pth.RemotePathLike
ListCmdArgs = List[CmdArg]
TupleCmdArgs = Tuple[CmdArg, ...]
CmdArgs = Union[ListCmdArgs, TupleCmdArgs]


class Environ(collections.abc.MutableMapping, metaclass=abc.ABCMeta):
  pass


class LocalEnviron(Environ):

  def __init__(self) -> None:
    self._environ = os.environ

  def __getitem__(self, key: str) -> str:
    return self._environ.__getitem__(key)

  def __setitem__(self, key: str, item: str) -> None:
    self._environ.__setitem__(key, item)

  def __delitem__(self, key: str) -> None:
    self._environ.__delitem__(key)

  def __iter__(self) -> Iterator[str]:
    return self._environ.__iter__()

  def __len__(self) -> int:
    return self._environ.__len__()


class SubprocessError(subprocess.CalledProcessError):
  """ Custom version that also prints stderr for debugging"""

  def __init__(self, platform: Platform, process) -> None:
    self.platform = platform
    super().__init__(process.returncode, shlex.join(map(str, process.args)),
                     process.stdout, process.stderr)

  def __str__(self) -> str:
    super_str = super().__str__()
    if not self.stderr:
      return f"{self.platform}: {super_str}"
    return f"{self.platform}: {super_str}\nstderr:{self.stderr.decode()}"


class Platform(abc.ABC):
  # pylint: disable=locally-disabled, redefined-builtin

  def __init__(self) -> None:
    self._binary_lookup_override: Dict[str, pth.RemotePath] = {}

  @property
  @abc.abstractmethod
  def name(self) -> str:
    pass

  @property
  @abc.abstractmethod
  def version(self) -> str:
    pass

  @property
  @abc.abstractmethod
  def device(self) -> str:
    pass

  @property
  @abc.abstractmethod
  def cpu(self) -> str:
    pass

  @property
  def full_version(self) -> str:
    return f"{self.name} {self.version} {self.machine}"

  def __str__(self) -> str:
    return ".".join(self.key) + (".remote" if self.is_remote else ".local")

  @property
  def is_remote(self) -> bool:
    return False

  @property
  def is_local(self) -> bool:
    return not self.is_remote

  @property
  def host_platform(self) -> Platform:
    return self

  @functools.cached_property
  def machine(self) -> MachineArch:
    raw = self._raw_machine_arch()
    if raw in ("i386", "i686", "x86", "ia32"):
      return MachineArch.IA32
    if raw in ("x86_64", "AMD64"):
      return MachineArch.X64
    if raw in ("arm64", "aarch64"):
      return MachineArch.ARM_64
    if raw in ("arm"):
      return MachineArch.ARM_32
    raise NotImplementedError(f"Unsupported machine type: {raw}")

  def _raw_machine_arch(self) -> str:
    assert self.is_local, "Unsupported operation on remote platform"
    return py_platform.machine()

  @property
  def is_ia32(self) -> bool:
    return self.machine == MachineArch.IA32

  @property
  def is_x64(self) -> bool:
    return self.machine == MachineArch.X64

  @property
  def is_arm64(self) -> bool:
    return self.machine == MachineArch.ARM_64

  @property
  def key(self) -> Tuple[str, str]:
    return (self.name, str(self.machine))

  @property
  def is_macos(self) -> bool:
    return False

  @property
  def is_linux(self) -> bool:
    return False

  @property
  def is_android(self) -> bool:
    return False

  @property
  def is_posix(self) -> bool:
    return self.is_macos or self.is_linux or self.is_android

  @property
  def is_win(self) -> bool:
    return False

  @property
  def is_remote_ssh(self) -> bool:
    return False

  @property
  def environ(self) -> Environ:
    assert self.is_local, "Unsupported operation on remote platform"
    return LocalEnviron()

  @property
  def is_battery_powered(self) -> bool:
    assert self.is_local, "Unsupported operation on remote platform"
    if not psutil.sensors_battery:
      return False
    status = psutil.sensors_battery()
    if not status:
      return False
    return not status.power_plugged

  def _search_executable(
      self,
      name: str,
      macos: Sequence[str],
      win: Sequence[str],
      linux: Sequence[str],
      lookup_callable: Callable[[pth.RemotePath], Optional[pth.RemotePath]],
  ) -> pth.RemotePath:
    executables: Sequence[str] = []
    if self.is_macos:
      executables = macos
    elif self.is_win:
      executables = win
    elif self.is_linux:
      executables = linux
    if not executables:
      raise ValueError(f"Executable {name} not supported on {self}")
    for name_or_path in executables:
      path = self.local_path(name_or_path).expanduser()
      binary = lookup_callable(path)
      if binary and self.exists(binary):
        return binary
    raise ValueError(f"Executable {name} not found on {self}")

  def search_app_or_executable(
      self,
      name: str,
      macos: Sequence[str] = (),
      win: Sequence[str] = (),
      linux: Sequence[str] = ()
  ) -> pth.RemotePath:
    return self._search_executable(name, macos, win, linux, self.search_app)

  def search_platform_binary(
      self,
      name: str,
      macos: Sequence[str] = (),
      win: Sequence[str] = (),
      linux: Sequence[str] = ()
  ) -> pth.RemotePath:
    return self._search_executable(name, macos, win, linux, self.search_binary)

  def search_app(self, app_or_bin: pth.RemotePath) -> Optional[pth.RemotePath]:
    """Look up a application bundle (macos) or binary (all other platforms) in
    the common search paths.
    """
    return self.search_binary(app_or_bin)

  @abc.abstractmethod
  def search_binary(self,
                    app_or_bin: pth.RemotePathLike) -> Optional[pth.RemotePath]:
    """Look up a binary in the common search paths based of a path or a single
    segment path with just the binary name.
    Returns the location of the binary (and not the .app bundle on macOS).
    """

  @abc.abstractmethod
  def app_version(self, app_or_bin: pth.RemotePathLike) -> str:
    pass

  @property
  def has_display(self) -> bool:
    """Return a bool whether the platform has an active display.
    This can be false on linux without $DISPLAY, true an all other platforms."""
    return True

  def sleep(self, seconds: Union[int, float, dt.timedelta]) -> None:
    if isinstance(seconds, dt.timedelta):
      seconds = seconds.total_seconds()
    if seconds == 0:
      return
    logging.debug("WAIT %ss", seconds)
    time.sleep(seconds)

  def which(self, binary_name: pth.RemotePathLike) -> Optional[pth.RemotePath]:
    if not binary_name:
      raise ValueError("Got empty path")
    assert self.is_local, "Unsupported operation on remote platform"
    if override := self.lookup_binary_override(binary_name):
      return override
    if result := shutil.which(binary_name):
      return self.path(result)
    return None

  def lookup_binary_override(
      self, binary_name: pth.RemotePathLike) -> Optional[pth.RemotePath]:
    return self._binary_lookup_override.get(str(binary_name))

  def set_binary_lookup_override(self, binary_name: pth.RemotePathLike,
                                 new_path: Optional[pth.RemotePath]):
    name = str(binary_name)
    if new_path is None:
      prev_result = self._binary_lookup_override.pop(name, None)
      if prev_result is None:
        logging.debug(
            "Could not remove binary override for %s as it was never set",
            binary_name)
      return
    if self.search_binary(new_path) is None:
      raise ValueError(f"Suggested binary override for {repr(name)} "
                       f"does not exist: {new_path}")
    self._binary_lookup_override[name] = new_path

  @contextlib.contextmanager
  def override_binary(self, binary: Union[pth.RemotePathLike, Binary],
                      result: Optional[pth.RemotePath]):
    binary_name: pth.RemotePathLike = ""
    if isinstance(binary, Binary):
      if override := binary.platform_path(self):
        binary_name = override
      else:
        raise RuntimeError("Cannot override binary:"
                           f" {binary} is not supported supported on {self}")
    else:
      binary_name = binary
    prev_override = self.lookup_binary_override(binary_name)
    self.set_binary_lookup_override(binary_name, result)
    try:
      yield
    finally:
      self.set_binary_lookup_override(binary_name, prev_override)

  def processes(self,
                attrs: Optional[List[str]] = None) -> List[Dict[str, Any]]:
    # TODO(cbruni): support remote platforms
    assert self.is_local, "Only local platform supported"
    return [
        p.info  # pytype: disable=attribute-error
        for p in psutil.process_iter(attrs=attrs)
    ]

  def process_running(self, process_name_list: List[str]) -> Optional[str]:
    assert self.is_local, "Unsupported operation on remote platform"
    # TODO(cbruni): support remote platforms
    for proc in psutil.process_iter():
      try:
        if proc.name().lower() in process_name_list:
          return proc.name()
      except (psutil.NoSuchProcess, psutil.AccessDenied, psutil.ZombieProcess):
        pass
    return None

  def process_children(self,
                       parent_pid: int,
                       recursive: bool = False) -> List[Dict[str, Any]]:
    assert self.is_local, "Unsupported operation on remote platform"
    # TODO(cbruni): support remote platforms
    try:
      process = psutil.Process(parent_pid)
    except (psutil.NoSuchProcess, psutil.AccessDenied, psutil.ZombieProcess):
      return []
    return [p.as_dict() for p in process.children(recursive=recursive)]

  def process_info(self, pid: int) -> Optional[Dict[str, Any]]:
    assert self.is_local, "Unsupported operation on remote platform"
    # TODO(cbruni): support remote platforms
    try:
      return psutil.Process(pid).as_dict()
    except psutil.NoSuchProcess:
      return None

  def foreground_process(self) -> Optional[Dict[str, Any]]:
    return None

  def terminate(self, proc_pid: int) -> None:
    assert self.is_local, "Unsupported operation on remote platform"
    # TODO(cbruni): support remote platforms
    process = psutil.Process(proc_pid)
    for proc in process.children(recursive=True):
      proc.terminate()
    process.terminate()

  @property
  def default_tmp_dir(self) -> pth.RemotePath:
    assert self.is_local, "Unsupported operation on remote platform"
    return self.path(tempfile.gettempdir())

  def reverse_port_forward(self, remote_port: int, local_port: int) -> None:
    if remote_port != local_port:
      raise ValueError("Cannot forward a remote port on a local platform.")
    assert self.is_local, "Unsupported operation on remote platform"

  def stop_reverse_port_forward(self, remote_port: int) -> None:
    del remote_port
    assert self.is_local, "Unsupported operation on remote platform"

  def cat(self, file: pth.RemotePathLike, encoding: str = "utf-8") -> str:
    """Meow! I return the file contents as a str."""
    with self.local_path(file).open(encoding=encoding) as f:
      return f.read()

  def set_file_contents(self,
                        file: pth.RemotePathLike,
                        data: str,
                        encoding: str = "utf-8") -> None:
    with self.local_path(file).open("w", encoding=encoding) as f:
      f.write(data)

  def rsync(self, from_path: pth.RemotePath,
            to_path: pth.LocalPath) -> pth.LocalPath:
    """ Convenience implementation that works for copying local dirs """
    assert self.is_local, "Unsupported operation on remote platform"
    if not self.exists(from_path):
      raise ValueError(f"Cannot copy non-existing source path: {from_path}")
    to_path.parent.mkdir(parents=True, exist_ok=True)
    if self.is_dir(from_path):
      shutil.copytree(from_path, to_path)
    else:
      shutil.copy2(from_path, to_path)
    return to_path

  def rm(self,
         path: pth.RemotePathLike,
         dir: bool = False,
         missing_ok: bool = False) -> None:
    """Remove a single file on this platform."""
    path = self.local_path(path)
    if dir:
      if missing_ok and not self.exists(path):
        return
      shutil.rmtree(path)
    else:
      path.unlink(missing_ok)

  def rename(self, src_path: pth.RemotePathLike,
             dst_path: pth.RemotePathLike) -> pth.RemotePath:
    """Remove a single file on this platform."""
    return self.local_path(src_path).rename(dst_path)

  def symlink_or_copy(self, src: pth.RemotePathLike,
                      dst: pth.RemotePathLike) -> pth.RemotePath:
    """Windows does not support symlinking without admin support.
    Copy files on windows (see WinPlatform) but symlink everywhere else."""
    assert not self.is_win, "Unsupported operation on windows"
    dst_path = self.local_path(dst)
    dst_path.symlink_to(self.path(src))
    return dst_path

  def path(self, path: pth.RemotePathLike) -> pth.RemotePath:
    """"Used to convert any paths and strings to a platform specific
    remote path.
    For instance a remote ADB platform on windows returns posix paths:
      posix_path = adb_remote_platform.patch(windows_path)
    This is used when passing out platform specific paths to remote shell
    commands.
    """
    return self.local_path(path)

  def local_path(self, path: pth.RemotePathLike) -> pth.LocalPath:
    assert self.is_local, "Unsupported operation on remote platform"
    return pth.LocalPath(path)

  def absolute(self, path: pth.RemotePathLike) -> pth.RemotePath:
    """Convert an arbitrary path to a platform-specific absolute path"""
    platform_path: pth.RemotePath = self.path(path)
    if platform_path.is_absolute():
      return platform_path
    if self.is_local:
      return self.local_path(platform_path).absolute()
    raise RuntimeError(
        f"Converting relative to absolute paths is not supported on {self}")

  def home(self) -> pth.RemotePath:
    return pathlib.Path.home()

  def touch(self, path: pth.RemotePathLike) -> None:
    self.local_path(path).touch(exist_ok=True)

  def mkdir(self,
            path: pth.RemotePathLike,
            parents: bool = True,
            exist_ok: bool = True) -> None:
    self.local_path(path).mkdir(parents=parents, exist_ok=exist_ok)

  def mkdtemp(self,
              prefix: Optional[str] = None,
              dir: Optional[pth.RemotePathLike] = None) -> pth.RemotePath:
    assert self.is_local, "Unsupported operation on remote platform"
    return self.path(tempfile.mkdtemp(prefix=prefix, dir=dir))

  def mktemp(self,
             prefix: Optional[str] = None,
             dir: Optional[pth.RemotePathLike] = None) -> pth.RemotePath:
    assert self.is_local, "Unsupported operation on remote platform"
    fd, name = tempfile.mkstemp(prefix=prefix, dir=dir)
    os.close(fd)
    return self.path(name)

  def exists(self, path: pth.RemotePathLike) -> bool:
    return self.local_path(path).exists()

  def is_file(self, path: pth.RemotePathLike) -> bool:
    return self.local_path(path).is_file()

  def is_dir(self, path: pth.RemotePathLike) -> bool:
    return self.local_path(path).is_dir()

  def iterdir(
      self, path: pth.RemotePathLike) -> Generator[pth.RemotePath, None, None]:
    return self.local_path(path).iterdir()

  def glob(self, path: pth.RemotePathLike,
           pattern: str) -> Generator[pth.RemotePath, None, None]:
    # TODO: support remotely
    return self.local_path(path).glob(pattern)

  def file_size(self, path: pth.RemotePathLike) -> int:
    # TODO: support remotely
    return self.local_path(path).stat().st_size

  def sh_stdout(self,
                *args: CmdArg,
                shell: bool = False,
                quiet: bool = False,
                encoding: str = "utf-8",
                env: Optional[Mapping[str, str]] = None,
                check: bool = True) -> str:
    completed_process = self.sh(
        *args,
        shell=shell,
        capture_output=True,
        quiet=quiet,
        env=env,
        check=check)
    return completed_process.stdout.decode(encoding)

  def popen(self,
            *args: CmdArg,
            shell: bool = False,
            stdout=None,
            stderr=None,
            stdin=None,
            env: Optional[Mapping[str, str]] = None,
            quiet: bool = False) -> subprocess.Popen:
    assert self.is_local, "Unsupported operation on remote platform"
    if not quiet:
      logging.debug("SHELL: %s", shlex.join(map(str, args)))
      logging.debug("CWD: %s", os.getcwd())
    return subprocess.Popen(
        args=args,
        shell=shell,
        stdin=stdin,
        stderr=stderr,
        stdout=stdout,
        env=env)

  def sh(self,
         *args: CmdArg,
         shell: bool = False,
         capture_output: bool = False,
         stdout=None,
         stderr=None,
         stdin=None,
         env: Optional[Mapping[str, str]] = None,
         quiet: bool = False,
         check: bool = True) -> subprocess.CompletedProcess:
    assert self.is_local, "Unsupported operation on remote platform"
    if not quiet:
      logging.debug("SHELL: %s", shlex.join(map(str, args)))
      logging.debug("CWD: %s", os.getcwd())
    process = subprocess.run(
        args=args,
        shell=shell,
        stdin=stdin,
        stdout=stdout,
        stderr=stderr,
        env=env,
        capture_output=capture_output,
        check=False)
    if check and process.returncode != 0:
      raise SubprocessError(self, process)
    return process

  def exec_apple_script(self, script: str) -> str:
    raise NotImplementedError("AppleScript is only available on MacOS")

  def log(self, *messages: Any, level: int = 2) -> None:
    message_str = " ".join(map(str, messages))
    if level == 3:
      level = logging.DEBUG
    if level == 2:
      level = logging.INFO
    if level == 1:
      level = logging.WARNING
    if level == 0:
      level = logging.ERROR
    logging.log(level, message_str)

  # TODO(cbruni): split into separate list_system_monitoring and
  # disable_system_monitoring methods
  def check_system_monitoring(self, disable: bool = False) -> bool:
    # pylint: disable=unused-argument
    return True

  def get_relative_cpu_speed(self) -> float:
    return 1

  def is_thermal_throttled(self) -> bool:
    return self.get_relative_cpu_speed() < 1

  def disk_usage(self, path: pth.RemotePathLike) -> psutil._common.sdiskusage:
    return psutil.disk_usage(str(self.local_path(path)))

  def cpu_usage(self) -> float:
    assert self.is_local, "Unsupported operation on remote platform"
    return 1 - psutil.cpu_times_percent().idle / 100

  def cpu_details(self) -> Dict[str, Any]:
    assert self.is_local, "Unsupported operation on remote platform"
    details = {
        "physical cores":
            psutil.cpu_count(logical=False),
        "logical cores":
            psutil.cpu_count(logical=True),
        "usage":
            psutil.cpu_percent(  # pytype: disable=attribute-error
                percpu=True, interval=0.1),
        "total usage":
            psutil.cpu_percent(),
        "system load":
            psutil.getloadavg(),
        "info":
            self.cpu,
    }
    try:
      cpu_freq = psutil.cpu_freq()
    except FileNotFoundError as e:
      logging.debug("psutil.cpu_freq() failed (normal on macOS M1): %s", e)
      return details
    details.update({
        "max frequency": f"{cpu_freq.max:.2f}Mhz",
        "min frequency": f"{cpu_freq.min:.2f}Mhz",
        "current frequency": f"{cpu_freq.current:.2f}Mhz",
    })
    return details

  def system_details(self) -> Dict[str, Any]:
    return {
        "machine": str(self.machine),
        "os": self.os_details(),
        "python": self.python_details(),
        "CPU": self.cpu_details(),
    }

  def os_details(self) -> JsonDict:
    assert self.is_local, "Unsupported operation on remote platform"
    return {
        "system": py_platform.system(),
        "release": py_platform.release(),
        "version": py_platform.version(),
        "platform": py_platform.platform(),
    }

  def python_details(self) -> JsonDict:
    assert self.is_local, "Unsupported operation on remote platform"
    return {
        "version": py_platform.python_version(),
        "bits": 64 if sys.maxsize > 2**32 else 32,
    }

  def download_to(self, url: str, path: pth.LocalPath) -> pth.LocalPath:
    assert self.is_local, "Unsupported operation on remote platform"
    logging.debug("DOWNLOAD: %s\n       TO: %s", url, path)
    assert not path.exists(), f"Download destination {path} exists already."
    try:
      urllib.request.urlretrieve(url, path)
    except (urllib.error.HTTPError, urllib.error.URLError) as e:
      raise OSError(f"Could not load {url}") from e
    assert path.exists(), (
        f"Downloading {url} failed. Downloaded file {path} doesn't exist.")
    return path

  def concat_files(self,
                   inputs: Iterable[pth.LocalPath],
                   output: pth.LocalPath,
                   prefix: str = "") -> pth.LocalPath:
    assert self.is_local, "Unsupported operation on remote platform"
    with output.open("w", encoding="utf-8") as output_f:
      if prefix:
        output_f.write(prefix)
      for input_file in inputs:
        assert input_file.is_file()
        with input_file.open(encoding="utf-8") as input_f:
          shutil.copyfileobj(input_f, output_f)
    return output

  def set_main_display_brightness(self, brightness_level: int) -> None:
    raise NotImplementedError(
        "Implementation is only available on MacOS for now")

  def get_main_display_brightness(self) -> int:
    raise NotImplementedError(
        "Implementation is only available on MacOS for now")

  def check_autobrightness(self) -> bool:
    raise NotImplementedError(
        "Implementation is only available on MacOS for now")

  def screenshot(self, result_path: pth.RemotePath) -> None:
    # TODO: support screen coordinates
    raise NotImplementedError(
        "Implementation is only available on MacOS for now")
