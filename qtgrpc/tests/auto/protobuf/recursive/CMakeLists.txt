# Copyright (C) 2023 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

qt6_add_protobuf(tst_protobuf_recursive_gen
    PROTO_FILES
        recursive.proto
    OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/qt_protobuf_generated"
)
qt_autogen_tools_initial_setup(tst_protobuf_recursive_gen)

qt_internal_add_test(tst_protobuf_recursive
    SOURCES
        tst_protobuf_recursive.cpp
    INCLUDE_DIRECTORIES
        ../shared
    LIBRARIES
        Qt::Test
        Qt::Protobuf
        tst_protobuf_recursive_gen
)
