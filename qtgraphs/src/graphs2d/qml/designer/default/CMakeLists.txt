# Copyright (C) 2024 The Qt Company Ltd.
# SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

qt_path_join(destination ${QT_INSTALL_DIR} "${INSTALL_QMLDIR}/QtGraphs/designer/default")
qt_copy_or_install(
    FILES
        BarSeries.qml
        LineSeries.qml
        ScatterSeries.qml
    DESTINATION "${destination}"
)
