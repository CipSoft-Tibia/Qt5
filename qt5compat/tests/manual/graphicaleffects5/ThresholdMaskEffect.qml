// Copyright (C) 2020 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

import QtQuick
import Qt5Compat.GraphicalEffects

Item {
    Image {
        id: mask
        source: "images/fog.png"
        sourceSize: Qt.size(parent.width, parent.height)
        smooth: true
        visible: false
    }

    ThresholdMask {
        anchors.fill: parent
        source: bug
        maskSource: mask
        threshold: 0.4
        spread: 0.2
    }
}
