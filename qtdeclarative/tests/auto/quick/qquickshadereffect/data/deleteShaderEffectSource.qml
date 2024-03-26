// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

import QtQuick 2.0
import QtQuick.Particles 2.0

Rectangle {
    color: "black"
    width: 320
    height: 320

    ShaderEffect {
        id: sei
        property variant source
    }

    ShaderEffectSource {
        id: doomed
        hideSource: true
        sourceItem: Image {
            source: "star.png"
        }
    }

    function setDeletedShaderEffectSource() {
        sei.source = doomed;
        doomed.destroy();
        // now set a fragment shader to trigger source texture detection.
        sei.fragmentShader = "qrc:/data/test.frag";
    }
}
