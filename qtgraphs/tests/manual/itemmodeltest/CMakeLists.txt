# Copyright (C) 2023 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

qt_internal_add_manual_test(tst_itemmodeltest
    GUI
    SOURCES
        main.cpp
    )
target_link_libraries(tst_itemmodeltest PUBLIC
    Qt::Gui
    Qt::Widgets
    Qt::Graphs
    )
