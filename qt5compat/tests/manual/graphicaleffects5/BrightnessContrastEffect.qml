// Copyright (C) 2020 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

import QtQuick
import Qt5Compat.GraphicalEffects

Item {
    BrightnessContrast {
        anchors.fill: parent
        source: bug
        brightness: 0.5
        contrast: 0.5
    }
}
