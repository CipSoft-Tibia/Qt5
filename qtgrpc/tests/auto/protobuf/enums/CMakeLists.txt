# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

qt6_add_protobuf(tst_protobuf_enumtypes_gen
    PROTO_FILES
        enummessages.proto
    OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/qt_protobuf_generated"
)
qt_autogen_tools_initial_setup(tst_protobuf_enumtypes_gen)

qt_internal_add_test(tst_protobuf_enumtypes
    SOURCES
        tst_protobuf_enumtypes.cpp
    INCLUDE_DIRECTORIES
        ../shared
    LIBRARIES
        Qt::Test
        Qt::Protobuf
        tst_protobuf_enumtypes_gen
)

qt_internal_add_test(tst_protobuf_serialization_enumtypes
    SOURCES
        tst_protobuf_serialization_enumtypes.cpp
    LIBRARIES
        Qt::Test
        Qt::Protobuf
        tst_protobuf_enumtypes_gen
)

qt_internal_add_test(tst_protobuf_deserialization_enumtypes
    SOURCES
        tst_protobuf_deserialization_enumtypes.cpp
    LIBRARIES
        Qt::Test
        Qt::Protobuf
        tst_protobuf_enumtypes_gen
)
