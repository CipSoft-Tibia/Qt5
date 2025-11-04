# Copyright 2023 The Chromium Authors
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

from __future__ import annotations

import abc
import functools
import logging
import re
from typing import TYPE_CHECKING, Any, Dict, Iterator, Optional

from crossbench import path as pth
from crossbench.plt.base import Environ, ListCmdArgs, Platform, SubprocessError
from crossbench.plt.remote import RemotePlatformMixin

if TYPE_CHECKING:
  from crossbench.types import JsonDict


class PosixPlatform(Platform, metaclass=abc.ABCMeta):
  # pylint: disable=locally-disabled, redefined-builtin

  def __init__(self) -> None:
    super().__init__()
    self._default_tmp_dir = pth.RemotePath("")

  @functools.cached_property
  def version(self) -> str:  #pylint: disable=invalid-overridden-method
    return self.sh_stdout("uname", "-r").strip()

  def _raw_machine_arch(self):
    if self.is_local:
      return super()._raw_machine_arch()
    return self.sh_stdout("uname", "-m").strip()

  def _get_cpu_cores_info(self) -> str:
    try:
      max_cores_file = self.path("/sys/devices/system/cpu/possible")
      _, max_core = self.cat(max_cores_file).strip().split("-", maxsplit=1)
      cores = int(max_core) + 1
      return f"{cores} cores"
    except Exception as e:  # pylint: disable=broad-except
      logging.debug("Failed to get detailed CPU stats: %s", e)
      return ""

  _GET_CPONF_PROC_RE: re.Pattern = re.compile(
      r".*PROCESSORS_CONF[^0-9]+(?P<cores>[0-9]+)")

  def cpu_details(self) -> Dict[str, Any]:
    if self.is_local:
      return super().cpu_details()
    cores = -1
    if self.which("nproc"):
      cores = int(self.sh_stdout("nproc"))
    elif self.which("getconf"):
      result = self._GET_CPONF_PROC_RE.search(self.sh_stdout("getconf", "-a"))
      if result:
        cores = int(result["cores"])
    return {
        "physical cores": cores,
        "info": self.cpu,
    }

  def os_details(self) -> JsonDict:
    if self.is_local:
      return super().os_details()
    return {
        "system": self.sh_stdout("uname").strip(),
        "release": self.sh_stdout("uname", "-r").strip(),
        "version": self.sh_stdout("uname", "-v").strip(),
        "platform": self.sh_stdout("uname", "-a").strip(),
    }

  _PY_VERSION: str = "import sys; print(64 if sys.maxsize > 2**32 else 32)"

  def python_details(self) -> JsonDict:
    if self.is_local:
      return super().python_details()
    if not self.which("python3"):
      return {"version": "unknown", "bits": 64}
    return {
        "version": self.sh_stdout("python3", "--version").strip(),
        "bits": int(self.sh_stdout("python3", "-c", self._PY_VERSION).strip())
    }

  def app_version(self, app_or_bin: pth.RemotePathLike) -> str:
    app_or_bin = self.path(app_or_bin)
    assert self.exists(app_or_bin), f"Binary {app_or_bin} does not exist."
    return self.sh_stdout(app_or_bin, "--version")

  @property
  def default_tmp_dir(self) -> pth.RemotePath:
    if self._default_tmp_dir.parts:
      return self._default_tmp_dir
    if self.is_local:
      self._default_tmp_dir = self.path(super().default_tmp_dir)
      return self._default_tmp_dir
    env = self.environ

    for tmp_var in ("TMPDIR", "TEMP", "TMP"):
      if tmp_var not in env:
        continue
      tmp_path = self.path(env[tmp_var])
      if self.is_dir(tmp_path):
        self._default_tmp_dir = tmp_path
        return tmp_path
    self._default_tmp_dir = self.path("/tmp")
    assert self.is_dir(self._default_tmp_dir), (
        f"Fallback tmp dir does not exist: {self._default_tmp_dir}")
    return self._default_tmp_dir

  def path(self, path: pth.RemotePathLike) -> pth.RemotePath:
    if self.is_local:
      return pth.LocalPosixPath(path)
    return pth.RemotePosixPath(path)

  def which(self, binary_name: pth.RemotePathLike) -> Optional[pth.RemotePath]:
    if self.is_local:
      return super().which(binary_name)
    if not binary_name:
      raise ValueError("Got empty path")
    if override := self.lookup_binary_override(str(binary_name)):
      return override
    try:
      if maybe_path := self.sh_stdout("which", self.path(binary_name)).strip():
        maybe_bin = self.path(maybe_path)
        if self.exists(maybe_bin):
          return maybe_bin
    except SubprocessError:
      pass
    return None

  def cat(self, file: pth.RemotePathLike, encoding: str = "utf-8") -> str:
    if self.is_local:
      return super().cat(file, encoding)
    return self.sh_stdout("cat", self.path(file), encoding=encoding)

  def rm(self,
         path: pth.RemotePathLike,
         dir: bool = False,
         missing_ok: bool = False) -> None:
    if self.is_local:
      super().rm(path, dir, missing_ok)
      return
    if missing_ok and not self.exists(path):
      return
    if dir:
      self.sh("rm", "-rf", self.path(path))
    else:
      self.sh("rm", self.path(path))

  def rename(self, src_path: pth.RemotePathLike,
             dst_path: pth.RemotePathLike) -> pth.RemotePath:
    if self.is_local:
      return super().rename(src_path, dst_path)
    dst_path = self.path(dst_path)
    self.sh("mv", self.path(src_path), dst_path)
    return dst_path

  def home(self) -> pth.RemotePath:
    if self.is_local:
      return super().home()
    return self.path(self.sh_stdout("printenv", "HOME").strip())

  def touch(self, path: pth.RemotePathLike) -> None:
    if self.is_local:
      super().touch(path)
    else:
      self.sh("touch", self.path(path))

  def mkdir(self,
            path: pth.RemotePathLike,
            parents: bool = True,
            exist_ok: bool = True) -> None:
    if self.is_local:
      super().mkdir(path, parents, exist_ok)
    elif parents or exist_ok:
      self.sh("mkdir", "-p", self.path(path))
    else:
      self.sh("mkdir", "-p", self.path(path))

  def mkdtemp(self,
              prefix: Optional[str] = None,
              dir: Optional[pth.RemotePathLike] = None) -> pth.RemotePath:
    if self.is_local:
      return super().mkdtemp(prefix, dir)
    return self._mktemp_sh(is_dir=True, prefix=prefix, dir=dir)

  def mktemp(self,
             prefix: Optional[str] = None,
             dir: Optional[pth.RemotePathLike] = None) -> pth.RemotePath:
    if self.is_local:
      return super().mktemp(prefix, dir)
    return self._mktemp_sh(is_dir=False, prefix=prefix, dir=dir)

  def _mktemp_sh(self, is_dir: bool, prefix: Optional[str],
                 dir: Optional[pth.RemotePathLike]) -> pth.RemotePath:
    if not dir:
      dir = self.default_tmp_dir
    template = self.path(dir) / f"{prefix}.XXXXXXXXXXX"
    args: ListCmdArgs = ["mktemp"]
    if is_dir:
      args.append("-d")
    args.append(str(template))
    result = self.sh_stdout(*args)
    return self.path(result.strip())

  def exists(self, path: pth.RemotePathLike) -> bool:
    if self.is_local:
      return super().exists(path)
    return self.sh("[", "-e", self.path(path), "]", check=False).returncode == 0

  def is_file(self, path: pth.RemotePathLike) -> bool:
    if self.is_local:
      return super().is_file(path)
    return self.sh("[", "-f", self.path(path), "]", check=False).returncode == 0

  def is_dir(self, path: pth.RemotePathLike) -> bool:
    if self.is_local:
      return super().is_dir(path)
    return self.sh("[", "-d", self.path(path), "]", check=False).returncode == 0

  def terminate(self, proc_pid: int) -> None:
    self.sh("kill", "-s", "TERM", str(proc_pid))

  def process_info(self, pid: int) -> Optional[Dict[str, Any]]:
    if self.is_local:
      return super().process_info(pid)
    try:
      lines = self.sh_stdout("ps", "-o", "comm", "-p", str(pid)).splitlines()
      if len(lines) <= 1:
        return None
      assert len(lines) == 2, lines
      tokens = lines[1].split()
      assert len(tokens) == 1
      return {"comm": tokens[0]}
    except SubprocessError:
      return None

  @property
  def environ(self) -> Environ:
    if self.is_local:
      return super().environ
    return RemotePosixEnviron(self)


class RemotePosixEnviron(Environ):

  def __init__(self, platform: PosixPlatform) -> None:
    self._platform = platform
    self._environ = {}
    for line in self._platform.sh_stdout("env").splitlines():
      parts = line.split("=", maxsplit=1)
      if len(parts) == 2:
        key, value = parts
        self._environ[key] = value
      else:
        assert len(parts) == 1
        key = parts[0]
        self._environ[key] = ""

  def __getitem__(self, key: str) -> str:
    return self._environ.__getitem__(key)

  def __setitem__(self, key: str, item: str) -> None:
    raise NotImplementedError("Unsupported")

  def __delitem__(self, key: str) -> None:
    raise NotImplementedError("Unsupported")

  def __iter__(self) -> Iterator[str]:
    return self._environ.__iter__()

  def __len__(self) -> int:
    return self._environ.__len__()


class RemotePosixPlatform(RemotePlatformMixin, PosixPlatform):
  pass