# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

if(TARGET WrapgRPC::WrapgRPCPlugin)
    set(WrapgRPCPlugin_FOUND TRUE)
    return()
endif()

find_program(__WrapgRPCPlugin_plugin_imported_location
        NAMES grpc_cpp_plugin grpc_cpp_plugin.exe
        # Support for vcpkg-based directory layout
        PATH_SUFFIXES tools/grpc
        HINTS "$ENV{gRPC_ROOT}/bin")

set(WrapgRPCPlugin_FOUND FALSE)

if(__WrapgRPCPlugin_plugin_imported_location)
    add_executable(WrapgRPC::WrapgRPCPlugin IMPORTED)
    set_target_properties(
        WrapgRPC::WrapgRPCPlugin
        PROPERTIES IMPORTED_LOCATION "${__WrapgRPCPlugin_plugin_imported_location}")
    set(WrapgRPCPlugin_FOUND TRUE)
endif()
