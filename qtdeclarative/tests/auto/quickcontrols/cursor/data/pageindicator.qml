// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

import QtQuick
import QtQuick.Controls

MouseArea {
    width: 200
    height: 200
    cursorShape: Qt.ForbiddenCursor

    PageIndicator {
        anchors.centerIn: parent
    }
}
