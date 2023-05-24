// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

import QtQuick

Item {
    id: top

    property string topText: "Mon"
    property string weatherIcon: "sunny"
    property string bottomText: "22*/23*"

    Text {
        id: dayText
        horizontalAlignment: Text.AlignHCenter
        width: top.width
        text: top.topText

        anchors.top: parent.top
        anchors.topMargin: top.height / 5 - dayText.paintedHeight
        anchors.horizontalCenter: parent.horizontalCenter
    }

    WeatherIcon {
        id: icon
        weatherIcon: top.weatherIcon

        property real side: {
            var h = 3 * top.height / 5
            if (top.width < h)
                return top.width;
            else
                return h;
        }

        width: icon.side
        height: icon.side

        anchors.centerIn: parent
    }

    Text {
        id: tempText
        horizontalAlignment: Text.AlignHCenter
        width: top.width
        text: top.bottomText

        anchors.bottom: parent.bottom
        anchors.bottomMargin: top.height / 5 - tempText.paintedHeight
        anchors.horizontalCenter: parent.horizontalCenter
    }
}
