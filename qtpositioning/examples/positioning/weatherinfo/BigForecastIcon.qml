// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

import QtQuick

Item {
    id: current

    property string topText: "20*"
    property string bottomText: "Mostly cloudy"
    property string weatherIcon: "sunny"
    property real smallSide: (current.width < current.height ? current.width : current.height)

    Text {
        text: current.topText
        font.pointSize: 28
        anchors {
            top: current.top
            left: current.left
            topMargin: 5
            leftMargin: 5
        }
    }

    WeatherIcon {
        weatherIcon: current.weatherIcon
        anchors.centerIn: parent
        anchors.verticalCenterOffset: -15
        width: current.smallSide
        height: current.smallSide
    }

    Text {
        text: current.bottomText
        font.pointSize: 23
        wrapMode: Text.WordWrap
        width: parent.width
        horizontalAlignment: Text.AlignRight
        anchors {
            bottom: current.bottom
            right: current.right
            rightMargin: 5
        }
    }
}
