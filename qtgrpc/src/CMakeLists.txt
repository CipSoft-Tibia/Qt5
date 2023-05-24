# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause
# Evaluate features before including repository subparts.

qt_feature_evaluate_features("${CMAKE_CURRENT_SOURCE_DIR}/protobuf/configure.cmake")
add_subdirectory(protobuf)

qt_feature_evaluate_features("${CMAKE_CURRENT_SOURCE_DIR}/grpc/configure.cmake")
if(QT_FEATURE_grpc)
    add_subdirectory(grpc)
endif()

add_subdirectory(tools)

if(TARGET Qt6::qtprotobufgen)
    if(QT_FEATURE_protobuf_wellknowntypes)
        add_subdirectory(wellknown)
    endif()
    add_subdirectory(protobufqttypes)
endif()
