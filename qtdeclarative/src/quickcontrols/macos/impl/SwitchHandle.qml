// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant reason:default

import QtQuick
import QtQuick.Templates as T
import QtQuick.Effects

Rectangle {
    id: handle
    implicitWidth: 22
    implicitHeight: 22
    radius: height / 2
    color: "transparent"
    border.color: Application.styleHints.accessibility.contrastPreference === Qt.HighContrast ? Application.styleHints.colorScheme === Qt.Light ? "#b3000000" : "#b3ffffff" : "transparent"

    required property bool down

    Rectangle {
        x: 1
        y: 1
        implicitWidth: 20
        implicitHeight: 20
        radius: 10
        color: Application.styleHints.colorScheme === Qt.Light
            ? Qt.darker(palette.base, handle.down ? 1.05 : 1)
            : Qt.lighter("#cdcbc9", handle.down ? 1.05 : 1)

        layer.enabled: Application.styleHints.accessibility.contrastPreference !== Qt.HighContrast
        layer.effect: MultiEffect {
            shadowEnabled: true
            blurMax: 10
            shadowBlur: 0.2
            shadowScale: 0.92
            shadowOpacity: 1
        }
    }
}
