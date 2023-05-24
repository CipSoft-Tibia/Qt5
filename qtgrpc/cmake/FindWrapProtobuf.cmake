# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

if(TARGET WrapProtobuf::WrapLibProtoc)
    set(WrapProtobuf_FOUND TRUE)
    return()
endif()

set(WrapProtobuf_FOUND FALSE)

set(__WrapProtobuf_find_package_args "")
if(${CMAKE_FIND_PACKAGE_NAME}_FIND_QUIETLY)
    list(APPEND __WrapProtobuf_find_package_args QUIET)
endif()

if(NOT TARGET Threads::Threads)
    # Protobuf needs Threads but doesn't do the lookup itself, so find
    # Threads first:
    find_package(Threads ${__WrapProtobuf_find_package_args})
endif()
if(TARGET Threads::Threads)
    qt_internal_disable_find_package_global_promotion(Threads::Threads)
endif()

# Protobuf can be represented in the system by both modern CONFIG and old style MODULE provided by
# CMake. The use of MODULE with new versions of protoc in PATH causes issues, so CONFIG should be
# preferred, but we still need to support MODULE. CMAKE_FIND_PACKAGE_PREFER_CONFIG gives this
# possibility.
set(__WrapProtobuf_CMAKE_FIND_PACKAGE_PREFER_CONFIG_save ${CMAKE_FIND_PACKAGE_PREFER_CONFIG})
set(CMAKE_FIND_PACKAGE_PREFER_CONFIG TRUE)
find_package(Protobuf ${WrapProtobuf_FIND_VERSION} ${__WrapProtobuf_find_package_args})
set(CMAKE_FIND_PACKAGE_PREFER_CONFIG ${__WrapProtobuf_CMAKE_FIND_PACKAGE_PREFER_CONFIG_save})
unset(__WrapProtobuf_CMAKE_FIND_PACKAGE_PREFER_CONFIG_save)

if(TARGET protobuf::protoc)
    qt_internal_disable_find_package_global_promotion(protobuf::protoc)
endif()

if(Protobuf_FOUND AND TARGET protobuf::libprotoc AND TARGET protobuf::libprotobuf)
    add_library(WrapProtobuf::WrapLibProtoc INTERFACE IMPORTED)
    target_link_libraries(WrapProtobuf::WrapLibProtoc INTERFACE protobuf::libprotoc)
    set_target_properties(WrapProtobuf::WrapLibProtoc PROPERTIES
        _qt_internal_protobuf_version "${Protobuf_VERSION}")

    add_library(WrapProtobuf::WrapLibProtobuf INTERFACE IMPORTED)
    target_link_libraries(WrapProtobuf::WrapLibProtobuf INTERFACE protobuf::libprotobuf)
    set_target_properties(WrapProtobuf::WrapLibProtobuf PROPERTIES
        _qt_internal_protobuf_version "${Protobuf_VERSION}")

    get_target_property(WrapProtobuf_INCLUDE_DIRS
        protobuf::libprotobuf
        INTERFACE_INCLUDE_DIRECTORIES
    )
    set_target_properties(WrapProtobuf::WrapLibProtobuf PROPERTIES
        _qt_internal_proto_include_dirs "${WrapProtobuf_INCLUDE_DIRS}"
    )
endif()

if(TARGET WrapProtobuf::WrapLibProtoc)
    set(WrapProtobuf_FOUND TRUE)
endif()

unset(__WrapProtobuf_find_package_args)
