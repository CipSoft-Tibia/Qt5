# Copyright (C) 2023 The Qt Company Ltd.
# SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

set(PROJECT_NAME "test_auto_callback")

qt_internal_add_test(${PROJECT_NAME}
    GUI
    QMLTEST
    SOURCES
        tst_callback.cpp
    LIBRARIES
        Qt::Core
        Qt::Qml
    TESTDATA
        tst_callback.qml
        PlaneScene.qml
    BUILTIN_TESTDATA
)

if(QT_BUILD_STANDALONE_TESTS)
    qt_import_qml_plugins(${PROJECT_NAME})
endif()
