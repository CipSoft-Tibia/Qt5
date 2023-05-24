# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

qt_internal_add_module(Protobuf
    SOURCES
        qtprotobufglobal.h
        qabstractprotobufserializer.cpp qabstractprotobufserializer.h
        qprotobufmessage.cpp qprotobufmessage.h qprotobufmessage_p.h
        qprotobuflazymessagepointer.h
        qprotobufobject.h
        qprotobufselfcheckiterator.h
        qprotobufserializer.cpp qprotobufserializer.h qprotobufserializer_p.h
        qtprotobuflogging.cpp qtprotobuflogging_p.h
        qtprotobuftypes.cpp qtprotobuftypes.h
        qprotobufoneof.cpp qprotobufoneof.h
    GENERATE_CPP_EXPORTS
    EXTRA_CMAKE_FILES
        "${CMAKE_CURRENT_SOURCE_DIR}/QtProtobufProperties.cmake.in"
    PUBLIC_LIBRARIES
        Qt::Core
    LIBRARIES
        Qt::CorePrivate
    PRIVATE_MODULE_INTERFACE
        Qt::CorePrivate
)

qt_internal_add_docs(Protobuf
    doc/qtprotobuf.qdocconf
)

# TODO: Remove this and fix it in qtbase's qt_internal_find_tool directly
if(QT_HOST_PATH AND Qt6::qtprotobufgen IN_LIST Qt6ProtobufTools_TARGETS
    AND NOT QT_FORCE_BUILD_TOOLS)
    qt_record_extra_qt_main_tools_package_dependency(Protobuf ProtobufTools "${PROJECT_VERSION}")
endif()
