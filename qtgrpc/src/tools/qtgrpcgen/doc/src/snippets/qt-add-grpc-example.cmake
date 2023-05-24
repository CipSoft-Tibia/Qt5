#! [0]
cmake_minimum_required(VERSION 3.16...3.22)
project(MyProject)

find_package(Qt6 REQUIRED COMPONENTS Protobuf Grpc)
qt_standard_project_setup()

qt_add_protobuf(MyProtoMessageLib
    PROTO_FILES
        path/to/helloworld.proto
    PROTO_INCLUDES
        path/to/proto/include
)

qt_add_grpc(MyGrpcClient CLIENT
    PROTO_FILES
        path/to/helloworld.proto
    PROTO_INCLUDES
        path/to/proto/include
)

qt_add_executable(MyApp main.cpp)

target_link_libraries(MyApp PRIVATE MyGrpcClient MyProtoMessageLib Qt6::Protobuf)
#! [0]
