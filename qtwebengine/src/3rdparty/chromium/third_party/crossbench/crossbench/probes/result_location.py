# Copyright 2024 The Chromium Authors
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

from __future__ import annotations

import enum

from crossbench import compat


@enum.unique
class ResultLocation(compat.StrEnumWithHelp):
  LOCAL = ("local",
           "Probe always produces results on the runner's local platform.")
  BROWSER = ("browser",
             ("Probe produces results on the browser's platform. "
              "This can be either remote (for instance android browser) "
              "or local (default system browser)."))
