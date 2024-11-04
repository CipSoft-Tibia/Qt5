# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

if(NOT QT_BUILD_STANDALONE_TESTS AND NOT QT_BUILDING_QT)
    cmake_minimum_required(VERSION 3.16)
    project(tst_qtgrpcgen LANGUAGES CXX)
    find_package(Qt6BuildInternals REQUIRED COMPONENTS STANDALONE_TEST)
endif()

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


qt_add_grpc(tst_qtgrpcgen_client_grpc_only CLIENT
    PROTO_FILES
        ../shared/data/proto/testservice.proto
    GENERATE_PACKAGE_SUBFOLDERS
    OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/qt_grpc_generated/separate/grpc"
)
qt_autogen_tools_initial_setup(tst_qtgrpcgen_client_grpc_only)

qt_add_protobuf(tst_qtgrpcgen_protobuf_only
    OUTPUT_DIRECTORY
        "${CMAKE_CURRENT_BINARY_DIR}/qt_grpc_generated/separate/protobuf"
    GENERATE_PACKAGE_SUBFOLDERS
    PROTO_FILES
        ../shared/data/proto/testservice.proto
)
qt_autogen_tools_initial_setup(tst_qtgrpcgen_protobuf_only)

target_link_libraries(tst_qtgrpcgen_client_grpc_only PRIVATE tst_qtgrpcgen_protobuf_only)

add_dependencies(tst_qtgrpcgen tst_qtgrpcgen_client_grpc_only)

if(TARGET Qt6::Qml)
    qt_internal_extend_target(tst_qtgrpcgen
        DEFINES
            HAVE_QML
    )

    qt_add_protobuf(tst_qtgrpcgen_plugin
        OUTPUT_DIRECTORY
            "${CMAKE_CURRENT_BINARY_DIR}/qt_grpc_generated_qml/qml"
        PROTO_FILES
            ../shared/data/proto/testservice.proto
    )

    qt_add_grpc(tst_qtgrpcgen_plugin CLIENT
        PROTO_FILES
            ../shared/data/proto/testservice.proto
        QML
        OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/qt_grpc_generated_qml/qml"
    )

    qt_autogen_tools_initial_setup(tst_qtgrpcgen_plugin)
    if(QT_BUILD_STANDALONE_TESTS)
        qt_import_qml_plugins(tst_qtgrpcgen_plugin)
    endif()

    add_dependencies(tst_qtgrpcgen tst_qtgrpcgen_plugin)
endif()
