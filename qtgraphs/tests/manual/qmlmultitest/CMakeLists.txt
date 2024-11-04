# Copyright (C) 2023 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

qt_internal_add_manual_test(tst_qmlmultitest
    GUI
    SOURCES
        main.cpp
    )
target_link_libraries(tst_qmlmultitest PUBLIC
    Qt::Gui
    Qt::Graphs
    )

set(qmlmultitest_resource_files
    "qml/qmlmultitest/Data.qml"
    "qml/qmlmultitest/main.qml"
    )

qt_internal_add_resource(tst_qmlmultitest "qmlmultitest"
    PREFIX
    "/"
    FILES
    ${qmlmultitest_resource_files}
    )
