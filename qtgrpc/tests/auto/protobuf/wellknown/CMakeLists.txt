# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

qt_internal_add_test(tst_protobuf_any
    SOURCES
        tst_protobuf_any.cpp
    LIBRARIES
        Qt::Test
        Qt::Protobuf
        Qt::ProtobufWellKnownTypes
)

qt6_add_protobuf(tst_protobuf_any
    PROTO_FILES
        ../../shared/data/proto/anymessages.proto
    OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/qt_protobuf_generated"
    PROTO_INCLUDES
        $<TARGET_PROPERTY:Qt6::ProtobufWellKnownTypes,QT_PROTO_INCLUDES>
)
qt_autogen_tools_initial_setup(tst_protobuf_basictypes_gen)
