# Copyright (C) 2023 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

cmake_minimum_required(VERSION 3.16)
project(GrpcChatClient LANGUAGES CXX)

if(NOT DEFINED INSTALL_EXAMPLESDIR)
    set(INSTALL_EXAMPLESDIR "examples")
endif()

set(INSTALL_EXAMPLEDIR "${INSTALL_EXAMPLESDIR}/grpc/chat")

find_package(Qt6 REQUIRED COMPONENTS Core Protobuf Grpc Quick)

qt_standard_project_setup()

qt_add_executable(grpcchatclient
    main.cpp
    simplechatengine.cpp simplechatengine.h
    chatmessagemodel.cpp chatmessagemodel.h
)

qt_add_qml_module(grpcchatclient
    URI qtgrpc.examples.chat
    VERSION 1.0
    RESOURCE_PREFIX "/qt/qml"
    QML_FILES
        Main.qml
        ChatInputField.qml
        ChatView.qml
)

qt_add_resources(grpcchatclient
    "assets"
    PREFIX "/"
    FILES
        "qt_logo_green_64x64px.png"
)

qt_add_protobuf(grpcchatclient
    PROTO_FILES
        ../proto/simplechat.proto
)

qt_add_grpc(grpcchatclient CLIENT
    PROTO_FILES
        ../proto/simplechat.proto
)

target_link_libraries(grpcchatclient
    PRIVATE
        Qt::Gui
        Qt::Quick
        Qt::Grpc
)

install(TARGETS grpcchatclient
    RUNTIME DESTINATION "${INSTALL_EXAMPLEDIR}"
    BUNDLE DESTINATION "${INSTALL_EXAMPLEDIR}"
    LIBRARY DESTINATION "${INSTALL_EXAMPLEDIR}"
)
