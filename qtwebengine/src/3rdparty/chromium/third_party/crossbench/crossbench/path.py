# Copyright 2024 The Chromium Authors
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import hashlib
import pathlib
import re
import unicodedata
from typing import Optional, Union

# A path that can refer to files on a remote platform with potentially
# a different Path flavour (e.g. Win vs Posix).
AnyPath = pathlib.PurePath
AnyPosixPath = pathlib.PurePosixPath
AnyWindowsPath = pathlib.PureWindowsPath

AnyPathLike = Union[str, AnyPath]

# A path that only ever refers to files on the local host / runner platform.
# Not that Path inherits from PurePath, and thus we can use a LocalPath in
# all places a RemotePath is expected.
LocalPath = pathlib.Path
LocalPosixPath = pathlib.PosixPath

LocalPathLike = Union[str, LocalPath]

_UNSAFE_FILENAME_CHARS_RE = re.compile(r"[^a-zA-Z0-9+\-_.]")


def safe_filename(name: str) -> str:
  normalized_name = unicodedata.normalize("NFKD", name)
  ascii_name = normalized_name.encode("ascii", "ignore").decode("ascii")
  return _UNSAFE_FILENAME_CHARS_RE.sub("_", ascii_name)


def try_resolve_existing_path(value: str) -> Optional[LocalPath]:
  if not value:
    return None
  maybe_path = LocalPath(value)
  if maybe_path.exists():
    return maybe_path
  maybe_path = maybe_path.expanduser()
  if maybe_path.exists():
    return maybe_path
  return None


def check_hash(file_path: LocalPath, file_hash: str) -> bool:
  if not file_path.exists():
    return False
  sha1 = hashlib.sha1()
  sha1.update(file_path.read_bytes())
  return sha1.hexdigest() == file_hash
