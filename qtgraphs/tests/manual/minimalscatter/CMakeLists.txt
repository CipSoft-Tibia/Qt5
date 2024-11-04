# Copyright (C) 2023 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

qt_internal_add_manual_test(tst_minimalscatter
    GUI
    SOURCES
        ../../../src/doc/snippets/doc_src_q3dscatter_construction.cpp
    )
target_link_libraries(tst_minimalscatter PUBLIC
    Qt::Gui
    Qt::Graphs
    )
