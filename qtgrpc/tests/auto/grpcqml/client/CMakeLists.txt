# Copyright (C) 2023 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

qt_internal_add_test(tst_client_qml
    QMLTEST
    SOURCES
        tst_client_qml.cpp
    LIBRARIES
        Qt6::Qml
        Qt6::Grpc
)

qt_add_grpc(tst_client_qml_gen CLIENT
    PROTO_FILES
        ../../shared/data/proto/testservice.proto
    QML
    OUTPUT_DIRECTORY
        "${CMAKE_CURRENT_BINARY_DIR}/qt_grpc_generated_qml"
)

qt_add_protobuf(tst_client_qml_gen
    PROTO_FILES
        ../../shared/data/proto/testservice.proto
    QML
    QML_URI qtgrpc.tests
    OUTPUT_DIRECTORY
        "${CMAKE_CURRENT_BINARY_DIR}/qt_grpc_generated_qml"
)

qt_policy(SET QTP0001 NEW)

qt_add_qml_module(tst_client_qml
    URI QmlTestUri
    VERSION 1.0
    QML_FILES
        qml/tst_client.qml
)

qt_autogen_tools_initial_setup(tst_client_qml)
qt_autogen_tools_initial_setup(tst_client_qml_gen)
qt_autogen_tools_initial_setup(tst_client_qml_genplugin)

if(QT_BUILD_STANDALONE_TESTS)
    qt_import_qml_plugins(tst_client_qml)
    qt_import_qml_plugins(tst_client_qml_gen)
endif()

