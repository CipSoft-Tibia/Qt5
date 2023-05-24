// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

import QtQuick

Item {
    id: root

    signal closed()

    property alias closingAnimation: closeAnimator
    property alias openingAnimation: openAnimator
    property alias animationText: result.text

    MagicText {
        id: result
        anchors.centerIn: parent

        font.pointSize: text.length > 12 ? 14 : 16
        color: "#2E53B6"

        ScaleAnimator on scale {
            id: openAnimator
            target: result
            from: 0
            to: 1
            duration: 2000
            running: false
        }

        ScaleAnimator on scale {
            id: closeAnimator
            target: result
            from: 1
            to: 0
            duration: 2000
            running: false
            onStopped: root.closed()
        }
    }
}
