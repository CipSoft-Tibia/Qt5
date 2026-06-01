# Copyright 2023 The Chromium Authors
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

from __future__ import annotations

from typing import Dict, List, Mapping, MutableMapping, Sequence, Tuple, Union

Json = Union["JsonMapping", "JsonSequence", str, int, float, bool, None]
JsonMapping = Mapping[str, Json]
JsonMutableMapping = MutableMapping[str, Json]
JsonDict = Dict[str, Json]
JsonSequence = Sequence[Json]
JsonList = List[Json]
JsonTuple = Tuple[Json, ...]
