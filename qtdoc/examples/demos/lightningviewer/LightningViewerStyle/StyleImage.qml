// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

import QtQuick

// This item will resize the child image in such a way that any drop shadow
// or blur (or other effects) will be drawn outside its own bounds.
// The effect is that users of this item won't have to take e.g shadows
// into account when positioning it, as such effects will only be visual, and
// not be a part of the geometry.

Item {
    id: root
    implicitWidth: horizontal ? imageConfig.width : imageConfig.height
    implicitHeight: horizontal ? imageConfig.height : imageConfig.width

    required property var imageConfig

    // Set horizontal to false if you want the image to be rotated 90 degrees
    // Doing so will rotate the image, but also flip it, to make sure that
    // the shadow ends up on the correct side. The implicit geometry of the
    // item will also be adjusted to match the rotated image.
    property bool horizontal: true

    // The minimum size of the image should be at least 1px tall and wide, even without any offsets
    property real minimumWidth: Math.max(1, imageConfig.leftOffset + imageConfig.rightOffset)
    property real minimumHeight: Math.max(1, imageConfig.topOffset + imageConfig.bottomOffset)

    BorderImage {
        x: -imageConfig.leftShadow
        y: -imageConfig.topShadow
        width: Math.max(root.minimumWidth, (root.horizontal ? root.width : root.height))
               + imageConfig.leftShadow + imageConfig.rightShadow
        height: Math.max(root.minimumHeight, (root.horizontal ? root.height : root.width))
                + imageConfig.topShadow + imageConfig.bottomShadow
        source: Qt.resolvedUrl(imageConfig.filePath)

        border {
            top: Math.min(height / 2, imageConfig.topOffset + imageConfig.topShadow)
            left: Math.min(width / 2, imageConfig.leftOffset + imageConfig.leftShadow)
            bottom: Math.min(height / 2, imageConfig.bottomOffset + imageConfig.bottomShadow)
            right: Math.min(width / 2, imageConfig.rightOffset + imageConfig.rightShadow)
        }

        transform: [
            Rotation {
                angle: root.horizontal ? 0 : 90
            },
            Scale {
                xScale: root.horizontal ? 1 : -1
            }
        ]
    }
}
