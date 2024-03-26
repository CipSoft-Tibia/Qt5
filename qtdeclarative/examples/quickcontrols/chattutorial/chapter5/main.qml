// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

import QtQuick
import QtQuick.Controls

ApplicationWindow {
    id: window
    width: 540
    height: 960
    visible: true

    StackView {
        id: stackView
        anchors.fill: parent
        initialItem: ContactPage {}
    }
}

