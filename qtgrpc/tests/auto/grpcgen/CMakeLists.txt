# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

qt_internal_add_test(tst_qtgrpcgen
    SOURCES
        tst_qtgrpcgen.cpp
    LIBRARIES
        Qt::Test
        Qt::Protobuf
        Qt::Grpc
    DEFINES
        PROTOC_EXECUTABLE="$<TARGET_FILE:WrapProtoc::WrapProtoc>"
)

qt_add_protobuf(tst_qtgrpcgen
    OUTPUT_DIRECTORY
        "${CMAKE_CURRENT_BINARY_DIR}/qt_grpc_generated/folder"
    GENERATE_PACKAGE_SUBFOLDERS
    PROTO_FILES
        ../shared/data/proto/testservice.proto
)

qt_add_grpc(tst_qtgrpcgen CLIENT
    PROTO_FILES
        ../shared/data/proto/testservice.proto
    GENERATE_PACKAGE_SUBFOLDERS
    OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/qt_grpc_generated/folder"
)

qt_autogen_tools_initial_setup(tst_qtgrpcgen)
