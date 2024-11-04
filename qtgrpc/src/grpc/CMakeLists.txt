# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

qt_internal_add_module(Grpc
    SOURCES
        qgrpcoperation.h qgrpcoperation.cpp
        qgrpcchanneloperation.h qgrpcchanneloperation.cpp
        qgrpccallreply.h qgrpccallreply.cpp
        qgrpcstream.h qgrpcstream.cpp
        qgrpcstatus.h qgrpcstatus.cpp
        qabstractgrpcchannel.h qabstractgrpcchannel_p.h qabstractgrpcchannel.cpp
        qgrpchttp2channel.h qgrpchttp2channel.cpp
        qabstractgrpcclient.h qabstractgrpcclient.cpp
        qgrpccalloptions.h qgrpccalloptions.cpp
        qgrpcchanneloptions.h qgrpcchanneloptions.cpp
        qgrpcclientinterceptor.h qgrpcclientinterceptor.cpp
        qgrpcclientinterceptormanager.h qgrpcclientinterceptormanager_p.h qgrpcclientinterceptormanager.cpp
        qgrpcmetadata.h
        qtgrpcglobal.h qtgrpcglobal_p.h  qtgrpcglobal.cpp
    GENERATE_CPP_EXPORTS
    LIBRARIES
        Qt::CorePrivate
        Qt::NetworkPrivate
    PUBLIC_LIBRARIES
        Qt::Core
        Qt::Protobuf
        Qt::Network
)

qt_internal_add_docs(Grpc
    doc/qtgrpc.qdocconf
)

# TODO: Remove this and fix it in qtbase's qt_internal_find_tool directly
if(QT_HOST_PATH AND Qt6::qtgrpcgen IN_LIST Qt6GrpcTools_TARGETS AND NOT QT_FORCE_BUILD_TOOLS)
    qt_record_extra_qt_main_tools_package_dependency(Grpc GrpcTools "${PROJECT_VERSION}")
endif()
