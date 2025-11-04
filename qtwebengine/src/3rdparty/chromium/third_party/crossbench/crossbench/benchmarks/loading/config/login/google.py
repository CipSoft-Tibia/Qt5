# Copyright 2024 The Chromium Authors
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

from __future__ import annotations

from crossbench.benchmarks.loading.config.login.custom import LoginBlock


class GoogleLogin(LoginBlock):
  """
  Google-specific login steps.
  """