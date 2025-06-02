# Copyright (C) 2024 The Qt Company Ltd.
# SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

cmake_minimum_required(VERSION 3.16)
project(VehicleServer LANGUAGES CXX)

set(CMAKE_FIND_PACKAGE_PREFER_CONFIG TRUE)

find_package(WrapProtoc)
find_package(WrapgRPC)
find_package(WrapgRPCPlugin)

if(NOT TARGET WrapProtoc::WrapProtoc OR NOT TARGET WrapgRPC::WrapLibgRPC
    OR NOT TARGET WrapgRPC::WrapgRPCPlugin)
    message(WARNING "Dependencies of VehicleServer not found. Skipping.")
    return()
endif()

if(MINGW)
    message(WARNING "${PROJECT_NAME} uses reference grpc++ library that doesn't officially support"
        " MinGW. Please use the MSVC compiler to build this example. The correct work is not"
        " guaranteed otherwise.")
endif()

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(proto_files
    "${CMAKE_CURRENT_LIST_DIR}/../proto/vehicleservice.proto"
    "${CMAKE_CURRENT_LIST_DIR}/../proto/navigationservice.proto")
set(out_dir ${CMAKE_CURRENT_BINARY_DIR})

set(generated_files
    "${out_dir}/vehicleservice.pb.h" "${out_dir}/vehicleservice.pb.cc"
    "${out_dir}/vehicleservice.grpc.pb.h" "${out_dir}/vehicleservice.grpc.pb.cc"
    "${out_dir}/navigationservice.pb.h" "${out_dir}/navigationservice.pb.cc"
    "${out_dir}/navigationservice.grpc.pb.h" "${out_dir}/navigationservice.grpc.pb.cc")

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

qt_add_executable(vehicle_server
    ${generated_files}
    ${CMAKE_CURRENT_LIST_DIR}/main.cpp
)

target_include_directories(vehicle_server
    PRIVATE
    ${out_dir}
)

target_link_libraries(vehicle_server
    PRIVATE
    WrapgRPC::WrapLibgRPC
)

install(TARGETS vehicle_server
    RUNTIME DESTINATION "${INSTALL_EXAMPLEDIR}"
    BUNDLE DESTINATION "${INSTALL_EXAMPLEDIR}"
    LIBRARY DESTINATION "${INSTALL_EXAMPLEDIR}"
)
