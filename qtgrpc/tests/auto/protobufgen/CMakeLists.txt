# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

if(NOT QT_BUILD_STANDALONE_TESTS AND NOT QT_BUILDING_QT)
    cmake_minimum_required(VERSION 3.16)
    project(tst_qtprotobufgen LANGUAGES CXX)
    find_package(Qt6BuildInternals REQUIRED COMPONENTS STANDALONE_TEST)
endif()

get_target_property(protoc_version WrapProtoc::WrapProtoc _qt_internal_protobuf_version)

qt_internal_add_test(tst_qtprotobufgen
    SOURCES
        tst_qtprotobufgen.cpp
    LIBRARIES
        Qt::Test
        Qt::Protobuf
    DEFINES
        PROTOC_EXECUTABLE="$<TARGET_FILE:WrapProtoc::WrapProtoc>"
)

qt_add_protobuf(tst_qtprotobufgen
    PROTO_FILES
        ../shared/data/proto/annotation.proto
    COPY_COMMENTS
    OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/qt_protobuf_generated/comments"
)

qt_add_protobuf(tst_qtprotobufgen
    PROTO_FILES
        ../shared/data/proto/fieldindexrange.proto
    OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/qt_protobuf_generated/fieldenum"
)

qt_add_protobuf(tst_qtprotobufgen
    PROTO_FILES
        ../shared/data/proto/extranamespace.proto
    EXTRA_NAMESPACE
        MyTopLevelNamespace
    OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/qt_protobuf_generated/extra-namespace"
)

add_library(tst_qtprotobufgen_gen SHARED)
qt_add_protobuf(tst_qtprotobufgen_gen
    PROTO_FILES
        ../shared/data/proto/basicmessages.proto
        ../shared/data/proto/mapmessages.proto
        ../shared/data/proto/oneofmessages.proto
        ../shared/data/proto/repeatedmessages.proto
    GENERATE_PACKAGE_SUBFOLDERS
    OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/qt_protobuf_generated/folder"
)

if(protoc_version VERSION_GREATER_EQUAL "3.12")
    qt_add_protobuf(tst_qtprotobufgen_gen
        PROTO_FILES
            ../shared/data/proto/optional.proto
        GENERATE_PACKAGE_SUBFOLDERS
        OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/qt_protobuf_generated/folder"
    )
endif()
qt_autogen_tools_initial_setup(tst_qtprotobufgen_gen)

add_library(tst_qtprotobufgen_custom_exports_gen SHARED)
qt_add_protobuf(tst_qtprotobufgen_custom_exports_gen
    PROTO_FILES
        ../shared/data/proto/basicmessages.proto
    EXPORT_MACRO "CUSTOM_EXPORT_NAME"
    OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/qt_protobuf_generated/custom-exports"
)
qt_autogen_tools_initial_setup(tst_qtprotobufgen_custom_exports_gen)

add_library(tst_qtprotobufgen_no_exports_gen STATIC)
qt_add_protobuf(tst_qtprotobufgen_no_exports_gen
    PROTO_FILES
        ../shared/data/proto/basicmessages.proto
    EXPORT_MACRO "CUSTOM_EXPORT_NAME"
    OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/qt_protobuf_generated/no-exports"
)
qt_autogen_tools_initial_setup(tst_qtprotobufgen_no_exports_gen)

qt_add_protobuf(tst_qtprotobufgen
    PROTO_FILES
        ../shared/data/proto/repeatednonpackedmessages.proto
    OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/qt_protobuf_generated/packed"
)
qt_autogen_tools_initial_setup(tst_qtprotobufgen)

if(MINGW)
    target_compile_options(tst_qtprotobufgen PRIVATE "-Wa,-mbig-obj")
    target_compile_options(tst_qtprotobufgen_gen PRIVATE "-Wa,-mbig-obj")
endif()

if(protoc_version VERSION_GREATER_EQUAL "3.12" AND protoc_version VERSION_LESS "3.15")
    qt_internal_extend_target(tst_qtprotobufgen
        DEFINES
            ALLOW_PROTO3_OPTIONAL
    )
endif()

add_dependencies(tst_qtprotobufgen tst_qtprotobufgen_gen tst_qtprotobufgen_custom_exports_gen)

if(TARGET Qt6::Qml)
    qt_internal_extend_target(tst_qtprotobufgen
        DEFINES
            HAVE_QML
    )

    qt_add_protobuf(tst_qtprotobufgen_qml_gen
        PROTO_FILES
        ../shared/data/proto/basicmessages.proto
        ../shared/data/proto/repeatedmessages.proto
        ../shared/data/proto/oneofmessages.proto
        ../protobuf/enums/enummessages.proto
        QML
        OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/qt_protobuf_generated/qmlgen"
    )
    qt_autogen_tools_initial_setup(tst_qtprotobufgen_qml_gen)
    qt_autogen_tools_initial_setup(tst_qtprotobufgen_qml_genplugin)

    qt_add_protobuf(tst_qtprotobufgen_nopackage_qml_gen
        PROTO_FILES
            ../shared/data/proto/nopackage.proto
            ../shared/data/proto/nopackageexternal.proto
        QML
        QML_URI "nopackage.uri.test"
        OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/qt_protobuf_generated/qml-no-package"
    )
    qt_autogen_tools_initial_setup(tst_qtprotobufgen_nopackage_qml_gen)
    qt_autogen_tools_initial_setup(tst_qtprotobufgen_nopackage_qml_genplugin)

    add_dependencies(tst_qtprotobufgen
        tst_qtprotobufgen_qml_gen
        tst_qtprotobufgen_nopackage_qml_gen
    )
endif()
