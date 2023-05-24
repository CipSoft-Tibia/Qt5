# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

qt_internal_add_module(Grpc
    SOURCES
        qgrpcoperation.h qgrpcoperation.cpp
        qgrpccallreply.h qgrpccallreply.cpp
        qgrpcstream.h qgrpcstream.cpp
        qgrpcstatus.h qgrpcstatus.cpp
        qabstractgrpcchannel.h qabstractgrpcchannel_p.h qabstractgrpcchannel.cpp
        qgrpchttp2channel.h qgrpchttp2channel.cpp
        qabstractgrpcclient.h qabstractgrpcclient.cpp
        qgrpccalloptions.h qgrpccalloptions.cpp
        qgrpcchanneloptions.h qgrpcchanneloptions.cpp
        qgrpcmetadata.h
        qtgrpcglobal.h qtgrpcglobal_p.h  qtgrpcglobal.cpp
    GENERATE_CPP_EXPORTS
    LIBRARIES
        Qt::CorePrivate
    PUBLIC_LIBRARIES
        Qt::Core
        Qt::Protobuf
        Qt::Network
)

qt_internal_extend_target(Grpc CONDITION QT_FEATURE_native_grpc
    SOURCES
        qgrpcchannel.h qgrpcchannel_p.h qgrpcchannel.cpp
    PUBLIC_LIBRARIES
        WrapgRPC::WrapLibgRPC
)

qt_internal_add_docs(Grpc
    doc/qtgrpc.qdocconf
)

# TODO: Remove this and fix it in qtbase's qt_internal_find_tool directly
if(QT_HOST_PATH AND Qt6::qtgrpcgen IN_LIST Qt6GrpcTools_TARGETS AND NOT QT_FORCE_BUILD_TOOLS)
    qt_record_extra_qt_main_tools_package_dependency(Grpc GrpcTools "${PROJECT_VERSION}")
endif()
