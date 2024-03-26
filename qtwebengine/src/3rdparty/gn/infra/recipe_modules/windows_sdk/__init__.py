# Copyright 2018 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

DEPS = [
    'recipe_engine/cipd',
    'recipe_engine/context',
    'recipe_engine/json',
    'recipe_engine/path',
    'recipe_engine/platform',
    'recipe_engine/step',
]

from recipe_engine.recipe_api import Property
from recipe_engine.config import ConfigGroup, Single

PROPERTIES = {
    '$gn/windows_sdk':
        Property(
            help='Properties specifically for the windows_sdk module.',
            param_name='sdk_properties',
            kind=ConfigGroup(
                # The CIPD package and version.
                sdk_package=Single(str),
                sdk_version=Single(str)),
            default={
                'sdk_package': 'chrome_internal/third_party/sdk/windows',
                'sdk_version': 'uploaded:2019-09-06'
            },
        )
}
