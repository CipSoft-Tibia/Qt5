// Copyright (C) 2020 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

import QtQuick
import Qt5Compat.GraphicalEffects

Item {
    LevelAdjust {
        anchors.fill: parent
        source: butterfly
        minimumOutput: "#00ffffff"
        maximumOutput: "#ff000000"
    }
}
