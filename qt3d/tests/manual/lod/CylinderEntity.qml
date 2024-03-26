// Copyright (C) 2014 Klaralvdalens Datakonsult AB (KDAB).
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

import QtQuick 2.4 as QQ2
import Qt3D.Core 2.0
import Qt3D.Render 2.0
import Qt3D.Extras 2.0

Entity {
    components: [ mesh, phongMaterial, transform ]

    CylinderMesh {
        id: mesh
        radius: 1
        length: 3
    }

    PhongMaterial {
        id: phongMaterial
        diffuse: "green"
    }

    Transform {
        id: transform
        scale: 4
    }
}
