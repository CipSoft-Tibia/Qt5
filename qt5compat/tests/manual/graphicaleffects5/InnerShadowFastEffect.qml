// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

import QtQuick
import Qt5Compat.GraphicalEffects

Item {
    InnerShadow {
        anchors.fill: parent
        source: butterfly
        horizontalOffset: -3
        verticalOffset: 3
        color: "#000000"
        radius: 32
        samples: 24
        spread: 0
        fast: true
    }
}
