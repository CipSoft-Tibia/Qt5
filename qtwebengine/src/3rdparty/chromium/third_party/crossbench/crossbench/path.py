# Copyright 2024 The Chromium Authors
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import pathlib
import re
import unicodedata
from typing import Union

# A path that can refer to files on a remote platform with potentially
# a different Path flavour (e.g. Win vs Posix).
RemotePath = pathlib.PurePath
RemotePosixPath = pathlib.PurePosixPath

RemotePathLike = Union[str, RemotePath]

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
