# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

qt6_add_protobuf(tst_protobuf_extranamespace_qtprotobuf_gen
    OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/qt_protobuf_generated"
    PROTO_FILES proto/extranamespace.proto
    EXTRA_NAMESPACE "MyTestNamespace"
)
qt_autogen_tools_initial_setup(tst_protobuf_extranamespace_qtprotobuf_gen)

qt_internal_add_test(tst_protobuf_extranamespace
    SOURCES
        tst_protobuf_extranamespace.cpp
    INCLUDE_DIRECTORIES
        ../shared
    LIBRARIES
        Qt::Test
        Qt::Protobuf
        tst_protobuf_extranamespace_qtprotobuf_gen
)
