// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

import QtQuick

Item {
    height: childrenRect.height

    // Display the number of lives
    Row {
        anchors.left: parent.left
        anchors.leftMargin: 10
        spacing: 5
        Repeater {
            id: rep
            model: Math.min(10, canvas.lives)
            delegate: Image { source: "gfx/lifes.png" }
        }
    }

    // Display the number of fishes saved
    Row {
        anchors.right: points.left
        anchors.rightMargin: 20
        spacing: 5
        Image { source: "gfx/scores.png" }
        Text {
            text: canvas.score
            font.bold: true
        }
    }

    // Display the number of coins
    Row {
        id: points
        anchors.right: parent.right
        anchors.rightMargin: 10
        spacing: 5
        Image { source: "gfx/points.png" }
        Text {
            id: pointsLabel
            text: canvas.coins
            font.bold: true
        }
    }
}

