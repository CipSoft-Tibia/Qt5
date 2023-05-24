# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

if(NOT QT_BUILD_STANDALONE_TESTS AND NOT QT_BUILDING_QT)
    cmake_minimum_required(VERSION 3.16)
    project(tst_protobuf_optional LANGUAGES CXX)
    find_package(Qt6BuildInternals REQUIRED COMPONENTS STANDALONE_TEST)
endif()

qt_internal_add_test(tst_protobuf_optional
    SOURCES
        tst_protobuf_optional.cpp
    INCLUDE_DIRECTORIES
        ../shared
    LIBRARIES
        Qt::Test
        Qt::Protobuf
)
qt_autogen_tools_initial_setup(tst_protobuf_optional)

get_target_property(protoc_version WrapProtoc::WrapProtoc _qt_internal_protobuf_version)
if(protoc_version VERSION_GREATER_EQUAL "3.12")
    qt6_add_protobuf(tst_protobuf_optional
        PROTO_FILES
            ../../shared/data/proto/optional.proto
    )
endif()
