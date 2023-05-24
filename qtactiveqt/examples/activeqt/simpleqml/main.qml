// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

import QtQuick 2.5
import QtQuick.Controls 2.0

import app 1.0

Rectangle {
    // Properties from context
    property Controller controller: context

    color: controller.color

    Label {
        id: idText
        text: "Color slider"
        anchors.top: parent.top
        anchors.left: parent.left
        color: "black"
        font.pixelSize: 12
    }

    Slider {
        value: controller.value
        onVisualPositionChanged: controller.value = visualPosition

        anchors.top: idText.bottom
        anchors.left: parent.left
    }
}
