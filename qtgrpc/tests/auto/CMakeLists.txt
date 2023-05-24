# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

if(QT_FEATURE_qtprotobufgen)
    add_subdirectory(protobufgen)
endif()
if(QT_FEATURE_qtgrpcgen)
    add_subdirectory(grpcgen)
endif()

add_subdirectory(protobuf)
if(QT_FEATURE_grpc AND TARGET Qt6::qtprotobufgen AND TARGET Qt6::qtgrpcgen)
    add_subdirectory(grpc)
endif()

if(TARGET Qt6::qtprotobufgen AND TARGET Qt6::Quick AND TARGET Qt6::QuickTest)
    add_subdirectory(protobufqml)
endif()

if(TARGET Qt6::qtgrpcgen AND TARGET Qt6::Quick AND TARGET Qt6::QuickTest)
    add_subdirectory(grpcqml)
endif()

if(TARGET Qt6::ProtobufQtCoreTypes
        AND TARGET Qt6::ProtobufQtGuiTypes
        AND TARGET Qt6::qtprotobufgen)
    add_subdirectory(protobufqttypes)
endif()
