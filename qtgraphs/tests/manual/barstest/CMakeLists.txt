# Copyright (C) 2023 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

set(CMAKE_INCLUDE_CURRENT_DIR ON)

qt_internal_add_manual_test(tst_barstest
    GUI
    SOURCES
        chart.cpp chart.h
        sliderwrapper.cpp sliderwrapper.h
        buttonwrapper.cpp buttonwrapper.h
        custominputhandler.cpp custominputhandler.h
        main.cpp
    )
target_link_libraries(tst_barstest PUBLIC
    Qt::Gui
    Qt::Widgets
    Qt::Graphs
    )

set(barstest_resource_files
    "shuttle.obj"
    "shuttle.png"
    )

qt_internal_add_resource(tst_barstest "barstest"
    PREFIX
    "/"
    FILES
    ${barstest_resource_files}
    )
