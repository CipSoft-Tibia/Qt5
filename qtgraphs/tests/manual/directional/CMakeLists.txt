# Copyright (C) 2023 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

set(CMAKE_INCLUDE_CURRENT_DIR ON)

qt_internal_add_manual_test(tst_directional
    GUI
    SOURCES
    main.cpp
    scatterdatamodifier.cpp scatterdatamodifier.h
    )
target_link_libraries(tst_directional PUBLIC
    Qt::Gui
    Qt::Widgets
    Qt::Graphs
    )
