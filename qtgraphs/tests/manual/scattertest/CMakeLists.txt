# Copyright (C) 2023 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

set(CMAKE_INCLUDE_CURRENT_DIR ON)

qt_internal_add_manual_test(tst_scattertest
    GUI
    SOURCES
        main.cpp
        scatterchart.cpp scatterchart.h
    )
target_link_libraries(tst_scattertest PUBLIC
    Qt::Gui
    Qt::Widgets
    Qt::Graphs
    )
