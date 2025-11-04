# Copyright (C) 2024 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

include(../../util/util.cmake)

qt_internal_add_test(tst_qwebengineprofilebuilder
    SOURCES
        tst_qwebengineprofilebuilder.cpp
    LIBRARIES
        Qt::WebEngineWidgets
)
