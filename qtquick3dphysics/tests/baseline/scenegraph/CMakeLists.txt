# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

#####################################################################
## tst_baseline_scenegraph Test:
#####################################################################

# Collect test data
list(APPEND test_data "../data")

qt_internal_add_test(tst_baseline_quick3dphysics
    OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/.."
    SOURCES
        ../shared/baselineprotocol.cpp
        ../shared/baselineprotocol.h
        ../shared/lookup3.cpp
        ../shared/qbaselinetest.cpp
        ../shared/qbaselinetest.h
        tst_baseline_quick3dphysics.cpp
    INCLUDE_DIRECTORIES
        ../shared
    LIBRARIES
        Qt::Gui
        Qt::Network
    TESTDATA ${test_data}
)
