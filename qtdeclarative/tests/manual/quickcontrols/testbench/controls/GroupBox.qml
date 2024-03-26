// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

import QtQuick
import QtQuick.Controls

QtObject {
    property var supportedStates: [
        [],
    ]

    property Component component: GroupBox {
        title: qsTr("Title")

        Label {
            text: qsTr("GroupBox")
        }
    }
}
