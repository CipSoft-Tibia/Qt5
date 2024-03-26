// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: BSD-3-Clause

import QtQuick

Item {
    id: rootItem
    property alias blurSrc1: blurredItemSource1
    property alias blurSrc2: blurredItemSource2
    property alias blurSrc3: blurredItemSource3
    property alias blurSrc4: blurredItemSource4
    property alias blurSrc5: blurredItemSource5

    component BlurItem : ShaderEffect {
        property vector2d offset: Qt.vector2d((1.0 + rootItem.blurMultiplier) / width,
                                              (1.0 + rootItem.blurMultiplier) / height)
        visible: false
        layer.enabled: true
        layer.smooth: true
        vertexShader: "bluritems.vert.qsb"
        fragmentShader: "bluritems.frag.qsb"
    }

    QtObject {
        id: priv
        property bool useBlurItem1: true
        property bool useBlurItem2: rootItem.blurMax > 2
        property bool useBlurItem3: rootItem.blurMax > 8
        property bool useBlurItem4: rootItem.blurMax > 16
        property bool useBlurItem5: rootItem.blurMax > 32
    }

    BlurItem {
        id: blurredItemSource1
        property Item src: priv.useBlurItem1 ? source : null
        // Size of the first blurred item is by default half of the source.
        // Increase for quality and decrease for performance & more blur.
        readonly property int blurItemSize: 8
        width: src ? Math.ceil(src.width / 16) * blurItemSize : 0
        height: src ? Math.ceil(src.height / 16) * blurItemSize : 0
    }
    BlurItem {
        id: blurredItemSource2
        property Item src: priv.useBlurItem2 ? blurredItemSource1 : null
        width: blurredItemSource1.width * 0.5
        height: blurredItemSource1.height * 0.5
    }
    BlurItem {
        id: blurredItemSource3
        property Item src: priv.useBlurItem3 ? blurredItemSource2 : null
        width: blurredItemSource2.width * 0.5
        height: blurredItemSource2.height * 0.5
    }
    BlurItem {
        id: blurredItemSource4
        property Item src: priv.useBlurItem4 ?  blurredItemSource3 : null
        width: blurredItemSource3.width * 0.5
        height: blurredItemSource3.height * 0.5
    }
    BlurItem {
        id: blurredItemSource5
        property Item src: priv.useBlurItem5 ? blurredItemSource4 : null
        width: blurredItemSource4.width * 0.5
        height: blurredItemSource4.height * 0.5
    }
}
