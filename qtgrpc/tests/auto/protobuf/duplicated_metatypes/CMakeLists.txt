# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

qt_internal_add_test(tst_protobuf_duplicatedmetatypes
    SOURCES
        tst_protobuf_duplicatedmetatypes.cpp
    INCLUDE_DIRECTORIES
        ../shared
    LIBRARIES
        Qt::Test
        Qt::Protobuf
)

qt6_add_protobuf(tst_protobuf_duplicatedmetatypes
    PROTO_FILES
        proto/duplicated_metatypes.proto
        proto/duplicated_metatypes_external.proto
    OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/qt_protobuf_generated"
)
qt_autogen_tools_initial_setup(tst_protobuf_duplicatedmetatypes)
