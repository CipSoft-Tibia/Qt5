# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

# Generated from qlinkedlist.pro.

#####################################################################
## tst_qlinkedlist Test:
#####################################################################

if(NOT QT_BUILD_STANDALONE_TESTS AND NOT QT_BUILDING_QT)
    cmake_minimum_required(VERSION 3.16)
    project(tst_qlinkedlist LANGUAGES CXX)
    find_package(Qt6BuildInternals REQUIRED COMPONENTS STANDALONE_TEST)
endif()

qt_internal_add_test(tst_qlinkedlist
    SOURCES
        tst_qlinkedlist.cpp
    LIBRARIES
        Qt::Core5Compat
)

qt_internal_undefine_global_definition(tst_qlinkedlist QT_NO_JAVA_STYLE_ITERATORS)
