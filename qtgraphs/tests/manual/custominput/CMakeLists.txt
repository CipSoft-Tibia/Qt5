# Copyright (C) 2023 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

set(CMAKE_INCLUDE_CURRENT_DIR ON)

qt_internal_add_manual_test(tst_custominput
    GUI
    SOURCES
        custominputhandler.cpp custominputhandler.h
        main.cpp
        scatterdatamodifier.cpp scatterdatamodifier.h
)

target_link_libraries(tst_custominput PUBLIC
    Qt::Core
    Qt::Gui
    Qt::Widgets
    Qt::Graphs
)

set(custominput_resource_files
    "data/data.txt"
)

qt6_add_resources(tst_custominput "custominput"
    PREFIX
        "/"
    FILES
        ${custominput_resource_files}
)
