// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

import QtQuick

Rectangle {
    id: delegate

    property bool selected: ListView.isCurrentItem
    property real itemSize
    width: itemSize
    height: itemSize

    Image {
        anchors.centerIn: parent
        source: image
    }

    Text {
        id: titleText

        anchors {
            left: parent.left; leftMargin: 20
            right: parent.right; rightMargin: 20
            top: parent.top; topMargin: 20
        }

        font { pixelSize: 18; bold: true }
        text: name
        color: selected ? "#ffffff" : "#ebebdd"
        scale: selected ? 1.15 : 1.0
        Behavior on color { ColorAnimation { duration: 150 } }
        Behavior on scale { PropertyAnimation { duration: 300 } }
    }

    BusyIndicator {
        scale: 0.8
        visible: delegate.ListView.isCurrentItem && window.loading
        anchors.centerIn: parent
    }

    MouseArea {
        anchors.fill: delegate
        onClicked: {
            delegate.ListView.view.currentIndex = index
            if (window.currentFeed == feed)
                feedModel.reload()
            else
                window.currentFeed = feed
        }
    }
}
