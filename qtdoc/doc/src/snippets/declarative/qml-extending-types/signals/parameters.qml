// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause
import QtQuick
//![0]
// application.qml
Button {
    width: 100; height: 100
    onButtonClicked: {
        console.log("Mouse clicked at " + xPos + "," + yPos)
    }
}
//![0]

