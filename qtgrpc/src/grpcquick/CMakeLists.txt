# Copyright (C) 2023 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

qt_internal_add_qml_module(GrpcQuick
    URI QtGrpc
    VERSION ${CMAKE_PROJECT_VERSION}
    PLUGIN_TARGET grpcquickplugin
    CLASS_NAME GrpcQuickPlugin
    SOURCES
        qqmlabstractgrpcchannel.h
        qqmlgrpchttp2channel_p.h qqmlgrpchttp2channel.cpp
        qqmlgrpcchanneloptions_p.h qqmlgrpcchanneloptions.cpp
        qqmlgrpccalloptions_p.h qqmlgrpccalloptions.cpp
        qqmlgrpcmetadata_p.h qqmlgrpcmetadata.cpp
    LIBRARIES
        Qt::Core
        Qt::Grpc
    PUBLIC_LIBRARIES
        Qt::Quick
    GENERATE_CPP_EXPORTS
)

qt_internal_extend_target(GrpcQuick CONDITION QT_FEATURE_qml_ssl
    LIBRARIES
        Qt::QmlNetwork
        Qt::QmlNetworkPrivate
)
