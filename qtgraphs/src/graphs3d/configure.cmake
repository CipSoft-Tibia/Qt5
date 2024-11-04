# Copyright (C) 2024 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

qt_feature("graphs-3d" PUBLIC
    LABEL "3D Graphs"
    PURPOSE "Support for 3D graphs"
    CONDITION TARGET Qt6::Quick3D
)

qt_feature("graphs-3d-bars3d" PUBLIC
    LABEL "Bars3D"
    PURPOSE "Support for Bars3D graph"
    CONDITION TARGET Qt6::Quick3D
)

qt_feature("graphs-3d-scatter3d" PUBLIC
    LABEL "Scatter3D"
    PURPOSE "Support for Scatter3D graph"
    CONDITION TARGET Qt6::Quick3D
)

qt_feature("graphs-3d-surface3d" PUBLIC
    LABEL "Surface3D"
    PURPOSE "Support for Surface3D graph"
    CONDITION TARGET Qt6::Quick3D
)
