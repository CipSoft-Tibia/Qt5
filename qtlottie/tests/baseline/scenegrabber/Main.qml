// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

import QtQuick
import Qt.labs.lottieqt

Image {
    id: chequered_background
    source: "qrc:///checkered.png"
    fillMode: Image.Tile
    width: qtlottie.width
    height: qtlottie.height

    LottieAnimation {
        id: qtlottie
        objectName: "qtlottie_animation_item"
        quality: LottieAnimation.HighQuality
        autoPlay: false

        property int freezeFrame: -1
        onStatusChanged: {
            if (status === LottieAnimation.Ready) {
                if (freezeFrame < 0)
                    freezeFrame = Math.floor((endFrame - startFrame) / 2);
                gotoAndStop(freezeFrame);
            }
        }
    }
}
