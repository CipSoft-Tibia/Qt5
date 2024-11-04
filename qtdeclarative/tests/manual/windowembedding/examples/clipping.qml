// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

import QtQuick

Rectangle {
    id: rootItem
    gradient: Gradient.TemptingAzure
    width: 500; height: 500

    Flickable {
        id: flickable
        anchors.fill: parent
        anchors.margins: 20
        contentWidth: 2000; contentHeight: 2000

        clip: true

        Window {
            id: redWindow
            flags: Qt.WindowTransparentForInput
            color: "lightgray"
            visible: true
            width: 200; height: 200
            parent: flickable.contentItem

            Image {
                source: "https://placedog.net/500/500?random"
                anchors.fill: parent
            }
        }
    }
}
