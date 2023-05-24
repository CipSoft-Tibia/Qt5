# Copyright (C) 2023 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

cmake_minimum_required(VERSION 3.16)

project(GrpcChatServer LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Qt6::Grpc module is not used directly in this project. But this allows to find Qt6::Grpc's
# dependencies without setting extra cmake module paths.
find_package(Qt6 COMPONENTS Grpc)
find_package(WrapgRPCPlugin)
find_package(WrapgRPC)

if(NOT TARGET WrapgRPC::WrapgRPCPlugin OR NOT TARGET WrapProtoc::WrapProtoc
    OR NOT TARGET WrapgRPC::WrapLibgRPC)
    message(WARNING "Dependencies of QtGrpc test server not found. Skipping.")
    return()
endif()

set(proto_files "${CMAKE_CURRENT_LIST_DIR}/../proto/simplechat.proto")
set(out_dir "${CMAKE_CURRENT_BINARY_DIR}")

set(generated_files
    "${out_dir}/simplechat.pb.h" "${out_dir}/simplechat.pb.cc"
    "${out_dir}/simplechat.grpc.pb.h" "${out_dir}/simplechat.grpc.pb.cc")

add_custom_command(
    OUTPUT ${generated_files}
    COMMAND
    $<TARGET_FILE:WrapProtoc::WrapProtoc>
    ARGS
    --grpc_out "${out_dir}"
    --cpp_out "${out_dir}"
    "-I${CMAKE_CURRENT_LIST_DIR}/../proto/"
    --plugin=protoc-gen-grpc=$<TARGET_FILE:WrapgRPC::WrapgRPCPlugin>
    "${proto_files}"
    WORKING_DIRECTORY ${out_dir}
    DEPENDS "${proto_files}"
    COMMENT "Generating gRPC ${target} sources..."
    COMMAND_EXPAND_LISTS
    VERBATIM
)

set_source_files_properties(${generated_files} PROPERTIES GENERATED TRUE)

add_executable(grpcchatserver
    ${generated_files}
    main.cpp
)

target_include_directories(grpcchatserver PRIVATE ${out_dir})
target_link_libraries(grpcchatserver PRIVATE WrapgRPC::WrapLibgRPC)

install(TARGETS grpcchatserver
    RUNTIME DESTINATION "${INSTALL_EXAMPLEDIR}"
    BUNDLE DESTINATION "${INSTALL_EXAMPLEDIR}"
    LIBRARY DESTINATION "${INSTALL_EXAMPLEDIR}"
)
