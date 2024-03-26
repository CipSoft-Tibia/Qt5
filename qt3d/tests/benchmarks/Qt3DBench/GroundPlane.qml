// Copyright (C) 2014 Klaralvdalens Datakonsult AB (KDAB).
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

import Qt3D.Core 2.0
import Qt3D.Render 2.0

Entity {
    id: root
    property Material material

    PlaneMesh {
        id: groundMesh
        width: 50
        height: width
        meshResolution: Qt.size(2, 2)
    }

    Transform {
        id: groundTransform
        Translate {
            dy: -14
        }
    }

    components: [
        groundMesh,
        groundTransform,
        material
    ]
}
