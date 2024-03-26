// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

import QtQuick
import QtQuick.Controls
import QtQuick.Controls.impl

AbstractButton {
    id: button
    width: 200
    height: 200
    icon.source: "qrc:/qt-project.org/imports/QtQuick/Controls/Basic/images/check.png"

    IconLabel {
        spacing: 10
        mirrored: true
        icon: button.icon
    }
}
