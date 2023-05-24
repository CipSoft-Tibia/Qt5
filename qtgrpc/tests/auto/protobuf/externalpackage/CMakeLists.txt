# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

qt_internal_add_test(tst_protobuf_externalpackage
    SOURCES
        tst_protobuf_externalpackage.cpp
    INCLUDE_DIRECTORIES
        ../shared
    LIBRARIES
        Qt::Test
        Qt::Protobuf
)

qt6_add_protobuf(tst_protobuf_externalpackage
    PROTO_FILES
        proto/externalpackage.proto
        proto/externalpackageconsumer.proto
    OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/qt_protobuf_generated"
)
qt_autogen_tools_initial_setup(tst_protobuf_externalpackage)
