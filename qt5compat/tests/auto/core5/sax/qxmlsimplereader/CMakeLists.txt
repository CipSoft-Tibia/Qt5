# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

# Generated from qxmlsimplereader.pro.

#####################################################################
## tst_qxmlsimplereader Test:
#####################################################################

# Collect test data
file(GLOB_RECURSE test_data_glob
    RELATIVE ${CMAKE_CURRENT_SOURCE_DIR}
    encodings/*)
list(APPEND test_data ${test_data_glob})
file(GLOB_RECURSE test_data_glob
    RELATIVE ${CMAKE_CURRENT_SOURCE_DIR}
    xmldocs/*)
list(APPEND test_data ${test_data_glob})

qt_internal_add_test(tst_qxmlsimplereader
    SOURCES
        parser/parser.cpp parser/parser.h
        tst_qxmlsimplereader.cpp
    INCLUDE_DIRECTORIES
        parser
    LIBRARIES
        Qt::Core5Compat
        Qt::Network
        Qt::Xml
    TESTDATA ${test_data}
)
