// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

import QtQuick3D
import QtQuick

CustomMaterial {
    property color uColor;
    shadingMode: CustomMaterial.Shaded
    vertexShader: "qrc:/shaders/vertexpointinstancing"
    fragmentShader: "qrc:/shaders/fragmentpoint"
}
