# Copyright (C) 2023 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

qt_internal_add_test(tst_protobuf_enumtypes_qml
    QMLTEST
    SOURCES
        tst_protobuf_enumtypes_qml.cpp
    LIBRARIES
        Qt::Qml
)

qt_policy(SET QTP0001 NEW)
qt_add_qml_module(tst_protobuf_enumtypes_qml
    URI QmlTestUri
    VERSION 1.0
    QML_FILES qml/tst_enumtypes.qml
)
qt_autogen_tools_initial_setup(tst_protobuf_enumtypes_qml)

qt_add_protobuf(tst_protobuf_enumtypes_qml_gen
    PROTO_FILES
        ../../protobuf/enums/enummessages.proto
    QML
    OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/qt_protobuf_generated"
)
qt_autogen_tools_initial_setup(tst_protobuf_enumtypes_qml_gen)
qt_autogen_tools_initial_setup(tst_protobuf_enumtypes_qml_genplugin)

qt_add_protobuf(tst_protobuf_additionalenumtypes_qml_gen
    PROTO_FILES
        ../../shared/data/proto/additionalenums.proto
    QML
    OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/qt_protobuf_generated_gen"
)
qt_autogen_tools_initial_setup(tst_protobuf_additionalenumtypes_qml_gen)
qt_autogen_tools_initial_setup(tst_protobuf_additionalenumtypes_qml_genplugin)

if(QT_BUILD_STANDALONE_TESTS)
    qt_import_qml_plugins(tst_protobuf_enumtypes_qml)
    qt_import_qml_plugins(tst_protobuf_enumtypes_qml_gen)
    qt_import_qml_plugins(tst_protobuf_additionalenumtypes_qml_gen)
endif()

