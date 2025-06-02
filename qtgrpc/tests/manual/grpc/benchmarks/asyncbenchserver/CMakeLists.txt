# Copyright (C) 2024 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(WrapgRPCPlugin)
find_package(WrapProtobuf)
find_package(WrapgRPC)

if(NOT WrapgRPCPlugin_FOUND OR NOT WrapProtobuf_FOUND OR NOT WrapgRPC_FOUND)
    message(WARNING "Dependencies of QtGrpc benchmark-server not found. Skipping.")
    return()
endif()

set(proto_out_dir "${CMAKE_CURRENT_BINARY_DIR}/include/proto/")
set(proto_out_include "${CMAKE_CURRENT_BINARY_DIR}/include/")

set(generated_files
    "${proto_out_dir}/bench.grpc.pb.h"
    "${proto_out_dir}/bench.grpc.pb.cc"
    "${proto_out_dir}/bench.pb.h"
    "${proto_out_dir}/bench.pb.cc"
)

set(extra_protoc_args "")
get_target_property(protoc_version WrapProtoc::WrapProtoc _qt_internal_protobuf_version)
if(protoc_version VERSION_GREATER_EQUAL "3.12" AND protoc_version VERSION_LESS "3.15")
    list(APPEND extra_protoc_args "--experimental_allow_proto3_optional")
endif()

add_custom_command(
    OUTPUT ${generated_files}
    COMMAND
        $<TARGET_FILE:WrapProtoc::WrapProtoc>
    ARGS
        "${extra_protoc_args}"
        --grpc_out "${proto_out_dir}"
        --cpp_out "${proto_out_dir}"
        -I "${proto_include}"
        --plugin=protoc-gen-grpc=$<TARGET_FILE:WrapgRPC::WrapgRPCPlugin>
        "${proto_path}"
    WORKING_DIRECTORY "${proto_out_dir}"
    DEPENDS
        "${proto_path}"
        $<TARGET_FILE:WrapProtoc::WrapProtoc>
        $<TARGET_FILE:WrapgRPC::WrapgRPCPlugin>
    COMMENT "Generating gRPC ${target} sources..."
    COMMAND_EXPAND_LISTS
    VERBATIM
)

set_source_files_properties(${generated_files} PROPERTIES GENERATED TRUE)

qt_internal_add_executable(
    asyncbenchserver
    SOURCES
        asyncbenchserver.cpp
        "${generated_files}"
    LIBRARIES
        WrapProtobuf::WrapLibProtobuf
        WrapgRPC::WrapLibgRPC
    INCLUDE_DIRECTORIES
        "${proto_out_include}"
        "${qrpcbench_common_include}"
    OUTPUT_DIRECTORY
        "${CMAKE_CURRENT_BINARY_DIR}"
)

