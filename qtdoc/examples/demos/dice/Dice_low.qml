// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

import QtQuick
import QtQuick3D

Node {
    id: node

    // Resources
    PrincipledMaterial {
        id: base_material
        baseColor: "#ffb8b8b8"
        roughness: 0.44999998807907104
        cullMode: PrincipledMaterial.NoCulling
        alphaMode: PrincipledMaterial.Opaque
    }
    PrincipledMaterial {
        id: dots_material
        baseColor: "#ff000000"
        roughness: 0.15000000596046448
        cullMode: PrincipledMaterial.NoCulling
        alphaMode: PrincipledMaterial.Opaque
    }

    // Nodes:
    Model {
        id: cube_001
        source: "meshes/cube_001.mesh"
        materials: [base_material, dots_material]
    }

    // Animations:
}
