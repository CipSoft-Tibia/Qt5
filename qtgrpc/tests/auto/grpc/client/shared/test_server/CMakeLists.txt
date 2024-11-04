# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

find_package(WrapgRPCPlugin)
find_package(WrapProtobuf)
find_package(WrapgRPC)

if(NOT WrapgRPCPlugin_FOUND OR NOT WrapProtobuf_FOUND OR NOT WrapgRPC_FOUND)
    message(WARNING "Dependencies of Qt GRPC test server not found. Skipping.")
    return()
endif()

set(proto_files "${CMAKE_CURRENT_LIST_DIR}/../../../shared/proto/testservice.proto")
set(out_dir ${CMAKE_CURRENT_BINARY_DIR})

set(generated_files
    "${out_dir}/testservice.pb.h" "${out_dir}/testservice.pb.cc"
    "${out_dir}/testservice.grpc.pb.h" "${out_dir}/testservice.grpc.pb.cc")

add_custom_command(
    OUTPUT ${generated_files}
    COMMAND
        $<TARGET_FILE:WrapProtoc::WrapProtoc>
    ARGS
        --grpc_out "${out_dir}"
        --cpp_out "${out_dir}"
        -I "${CMAKE_CURRENT_LIST_DIR}/../../../shared/proto/"
        --plugin=protoc-gen-grpc=$<TARGET_FILE:WrapgRPC::WrapgRPCPlugin>
        "${proto_files}"
    WORKING_DIRECTORY ${out_dir}
    DEPENDS "${proto_files}" $<TARGET_FILE:WrapProtoc::WrapProtoc>
    COMMENT "Generating gRPC ${target} sources..."
    COMMAND_EXPAND_LISTS
    VERBATIM
)

set_source_files_properties(${generated_files} PROPERTIES GENERATED TRUE)
add_library(grpc_testserverrunner_gen STATIC ${generated_files})
target_include_directories(grpc_testserverrunner_gen
    PRIVATE
        ${out_dir}
        WrapgRPC_INCLUDE_PATH
)

target_link_libraries(grpc_testserverrunner_gen
    PRIVATE
        WrapProtobuf::WrapLibProtobuf
        WrapgRPC::WrapLibgRPC
)

add_library(grpc_testserverrunner
    STATIC
        ${CMAKE_CURRENT_LIST_DIR}/testserverrunner.cpp
        ${CMAKE_CURRENT_LIST_DIR}/testserverrunner.h
)

target_include_directories(grpc_testserverrunner
    PRIVATE
        ${out_dir}
        ${CMAKE_CURRENT_LIST_DIR}/../../../shared
)

target_link_libraries(grpc_testserverrunner
    PRIVATE
        grpc_testserverrunner_gen
        WrapgRPC::WrapLibgRPC
        Qt6::Core
)

set_source_files_properties("${CMAKE_CURRENT_LIST_DIR}/assets/cert.pem"
    PROPERTIES QT_RESOURCE_ALIAS assets/cert.pem)
set_source_files_properties("${CMAKE_CURRENT_LIST_DIR}/assets/key.pem"
    PROPERTIES QT_RESOURCE_ALIAS assets/key.pem)

qt_add_resources(grpc_testserverrunner
    "keys"
    PREFIX
        "/"
    FILES
        "${CMAKE_CURRENT_LIST_DIR}/assets/cert.pem"
        "${CMAKE_CURRENT_LIST_DIR}/assets/key.pem"
)

qt_internal_add_executable(grpc_testserver
    SOURCES
        ${CMAKE_CURRENT_LIST_DIR}/testserver.cpp
    LIBRARIES
        grpc_testserverrunner
    OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/"
    INSTALL_DIRECTORY "${INSTALL_TESTSDIR}/shared"
)
