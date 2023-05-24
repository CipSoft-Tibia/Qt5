// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

import QtQuick

Rectangle {
    id: root

    property bool runAnimation: false

    anchors.centerIn: parent
    width: 100
    height: width

    color: "transparent"

    Row {
        id: scene
        anchors.centerIn: parent
        spacing: 12

        Repeater {
            model: 4
            ProgressDot {}
        }
    }

    ScaleAnimator on scale {
        id: openning
        target: root
        from: 0.3
        to: 1
        duration: 1000
        running: root.runAnimation
        onStopped: closing.start()
        easing.amplitude: 6.0
        easing.period: 2.5
    }

    ScaleAnimator on scale {
        id: closing
        target: root
        from: 1
        to: 0.3
        duration: 1000
        running: false
        onStopped: {
            if (root.runAnimation)
                openning.start()
        }
        easing.amplitude: 6.0
        easing.period: 2.5
    }
}
