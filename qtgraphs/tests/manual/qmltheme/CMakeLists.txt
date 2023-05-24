# Copyright (C) 2023 The Qt Company Ltd.
# SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

set(CMAKE_INCLUDE_CURRENT_DIR ON)

qt_internal_add_manual_test(tst_qmltheme
    GUI
    SOURCES
        main.cpp
    )

target_link_libraries(tst_qmltheme PUBLIC
    Qt::Core
    Qt::Gui
    Qt::Qml
    Qt::Quick
    Qt::Graphs
)

set(qmltheme_resource_files
    "qml/qmltheme/main.qml"
    "layer_1.png"
)

qt6_add_resources(tst_qmltheme "qmltheme"
    PREFIX
        "/"
    FILES
        ${qmltheme_resource_files}
)
