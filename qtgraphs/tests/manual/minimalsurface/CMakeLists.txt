# Copyright (C) 2023 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

qt_internal_add_manual_test(tst_minimalsurface
    GUI
    SOURCES
        ../../../src/doc/snippets/doc_src_q3dsurface_construction.cpp
    )
target_link_libraries(tst_minimalsurface PUBLIC
    Qt::Gui
    Qt::Graphs
    )
