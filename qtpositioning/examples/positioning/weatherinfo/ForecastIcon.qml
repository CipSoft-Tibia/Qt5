// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

import QtQuick

Item {
    id: top

    property string topText: "Mon"
    property string middleIcon: "sunny"
    property string bottomText: "22*/23*"

    implicitHeight: dayText.implicitHeight + width + tempText.implicitHeight + 20
    Text {
        id: dayText
        horizontalAlignment: Text.AlignHCenter
        width: parent.width
        text: top.topText
        color: "white"
        font.pixelSize: 24

        anchors.top: parent.top
        anchors.margins: 10
        anchors.horizontalCenter: parent.horizontalCenter
    }

    WeatherIcon {
        id: icon
        weatherIcon: top.middleIcon

        width: height
        anchors.top: dayText.bottom
        anchors.bottom: tempText.top
        anchors.margins: 10
        anchors.horizontalCenter: parent.horizontalCenter
    }

    Text {
        id: tempText
        horizontalAlignment: Text.AlignHCenter
        width: top.width
        text: top.bottomText
        font.pixelSize: 16

        anchors.bottom: parent.bottom
        anchors.margins: 10
        anchors.horizontalCenter: parent.horizontalCenter
        color: "white"

    }
}
