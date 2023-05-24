# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

# Generated from qtextcodec.pro.

#####################################################################
## tst_qtextcodec Test:
#####################################################################

# Collect test data
file(GLOB_RECURSE test_data_glob
    RELATIVE ${CMAKE_CURRENT_SOURCE_DIR}
    *.txt)
list(APPEND test_data ${test_data_glob})

qt_internal_add_test(tst_qtextcodec
    SOURCES
        tst_qtextcodec.cpp
    LIBRARIES
        Qt::Core5Compat
    TESTDATA ${test_data}
)
if(UNIX)
    add_subdirectory(echo)
endif()
