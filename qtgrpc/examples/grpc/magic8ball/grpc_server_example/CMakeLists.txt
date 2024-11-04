# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

cmake_minimum_required(VERSION 3.16)
project(MagicServerRunner LANGUAGES CXX)

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

set(proto_files "${CMAKE_CURRENT_LIST_DIR}/../proto/exampleservice.proto")
set(out_dir ${CMAKE_CURRENT_BINARY_DIR})

set(generated_files
    "${out_dir}/exampleservice.pb.h" "${out_dir}/exampleservice.pb.cc"
    "${out_dir}/exampleservice.grpc.pb.h" "${out_dir}/exampleservice.grpc.pb.cc")

add_custom_command(
    OUTPUT ${generated_files}
    COMMAND
    $<TARGET_FILE:WrapProtoc::WrapProtoc>
    ARGS
    --grpc_out "${out_dir}"
    --cpp_out "${out_dir}"
    -I "${CMAKE_CURRENT_LIST_DIR}/../proto/"
    --plugin=protoc-gen-grpc=$<TARGET_FILE:WrapgRPC::WrapgRPCPlugin>
    "${proto_files}"
    WORKING_DIRECTORY ${out_dir}
    DEPENDS "${proto_files}"
    COMMENT "Generating gRPC ${target} sources..."
    COMMAND_EXPAND_LISTS
    VERBATIM
)

set_source_files_properties(${generated_files} PROPERTIES GENERATED TRUE)
add_library(ServerRunner_grpc_gen STATIC ${generated_files})
target_include_directories(ServerRunner_grpc_gen
    PRIVATE
    ${out_dir}
)

target_link_libraries(ServerRunner_grpc_gen
    PRIVATE
    WrapgRPC::WrapLibgRPC
)

add_library(MagicServerRunner
    STATIC
    serverrunner.cpp
    serverrunner.h
)

target_include_directories(MagicServerRunner PRIVATE ${out_dir})

target_link_libraries(MagicServerRunner
    PRIVATE
    ServerRunner_grpc_gen
    WrapgRPC::WrapLibgRPC
    Qt6::Core
)

qt_add_executable(SimpleGrpcServer
    main.cpp
)

target_link_libraries(SimpleGrpcServer PRIVATE
    Qt6::Core
    MagicServerRunner
)

install(TARGETS SimpleGrpcServer
    RUNTIME DESTINATION "${INSTALL_EXAMPLEDIR}"
    BUNDLE DESTINATION "${INSTALL_EXAMPLEDIR}"
    LIBRARY DESTINATION "${INSTALL_EXAMPLEDIR}"
)
