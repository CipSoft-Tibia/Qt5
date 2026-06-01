// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

import QtQuick
import Qt.labs.lottieqt
import QtQuick.VectorImage

Image {
    id: chequered_background
    source: "qrc:///checkered.png"
    fillMode: Image.Tile
    width: qtlottie.width + 5
    height: qtlottie.height * row.scale

    Row {
        id: row
        anchors.top: parent.top
        anchors.left: parent.left
        scale: 0.5
        transformOrigin: Item.TopLeft

        LottieAnimation {
            id: qtlottie
            objectName: "qtlottie_animation_item"
            quality: LottieAnimation.HighQuality
            autoPlay: false
            property int freezeFrame: -1
            onStatusChanged: {
                if (status === LottieAnimation.Ready) {
                    if (freezeFrame < 0)
                        freezeFrame = Math.floor(startFrame + ((endFrame - startFrame) / 2));
                    gotoAndStop(freezeFrame);
                }
            }
            clip: true
        }

        Rectangle {
            width: 5
            height: qtlottie.height
            color: "black"
        }

        VectorImage {
            assumeTrustedSource: true
            animations.paused: true
            source: qtlottie.source
            preferredRendererType: VectorImage.CurveRenderer
            clip: true
        }
    }
}
