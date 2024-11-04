# Copyright (C) 2023 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

qt_internal_add_manual_test(tst_qmlgradient
    GUI
    SOURCES
        main.cpp
    )
target_link_libraries(tst_qmlgradient PUBLIC
    Qt::Gui
    Qt::Graphs
    )

set(qmlgradient_resource_files
    "qml/qmlgradient/main.qml"
    "crater.png"
    )

set_source_files_properties("crater.png"
    PROPERTIES QT_RESOURCE_ALIAS "map"
    )

qt_internal_add_resource(tst_qmlgradient "qmlgradient"
    PREFIX
    "/"
    FILES
    ${qmlgradient_resource_files}
    )
