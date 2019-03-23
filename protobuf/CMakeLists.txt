# Copyright 2018 gRPC authors.
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#     http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
#
# cmake build file for C++ helloworld example.
# Assumes protobuf and gRPC have been installed using cmake.
# See cmake_externalproject/CMakeLists.txt for all-in-one cmake build
# that automatically builds all the dependencies before building helloworld.

cmake_minimum_required(VERSION 2.8)

message(">>>> ${CMAKE_CURRENT_SOURCE_DIR}")

#if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/../CMakeModules)
#    set(CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/../CMakeModules)
#endif()

include("HunterGate")
#HunterGate(
#        URL "https://github.com/ruslo/hunter/archive/v0.23.31.tar.gz"
#        SHA1 "3d94b8522541c4c8eaac606f4c47d09322a8019c"
#)
HunterGate(
        URL "https://github.com/ruslo/hunter/archive/v0.23.114.tar.gz"
        SHA1 "712b3f5ea467fbddd4769a7800a060fbd6a31ab5"
)

project(grpc_example)

#hunter_add_package(Beast)
#find_package(Beast CONFIG REQUIRED)

hunter_add_package(Boost)
find_package(Boost CONFIG REQUIRED)

hunter_add_package(nlohmann_fifo_map)
find_package(nlohmann_fifo_map CONFIG REQUIRED)


hunter_add_package(Protobuf)
find_package(Protobuf CONFIG REQUIRED)

hunter_add_package(gRPC)
find_package(gRPC CONFIG REQUIRED)

if(NOT MSVC)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")
else()
    add_definitions(-D_WIN32_WINNT=0x600)
endif()

# This branch assumes that gRPC and all its dependencies are already installed
# on this system, so they can be located by find_package().

# Find Protobuf installation
# Looks for protobuf-config.cmake file installed by Protobuf's cmake installation.
set(protobuf_MODULE_COMPATIBLE TRUE)
find_package(Protobuf CONFIG REQUIRED)
message(STATUS "Using protobuf ${protobuf_VERSION}")

set(_PROTOBUF_LIBPROTOBUF protobuf::libprotobuf)
set(_PROTOBUF_PROTOC $<TARGET_FILE:protobuf::protoc>)

# Find gRPC installation
# Looks for gRPCConfig.cmake file installed by gRPC's cmake installation.
find_package(gRPC CONFIG REQUIRED)
message(STATUS "Using gRPC ${gRPC_VERSION}")

set(_GRPC_GRPCPP_UNSECURE gRPC::grpc++_unsecure)
set(_GRPC_CPP_PLUGIN_EXECUTABLE $<TARGET_FILE:gRPC::grpc_cpp_plugin>)


# Proto file
get_filename_component(hw_proto "route_guide.proto" ABSOLUTE)
get_filename_component(hw_proto_path "${hw_proto}" PATH)

# Generated sources
set(hw_proto_srcs "${CMAKE_CURRENT_BINARY_DIR}/route_guide.pb.cc")
set(hw_proto_hdrs "${CMAKE_CURRENT_BINARY_DIR}/route_guide.pb.h")
set(hw_grpc_srcs "${CMAKE_CURRENT_BINARY_DIR}/route_guide.grpc.pb.cc")
set(hw_grpc_hdrs "${CMAKE_CURRENT_BINARY_DIR}/route_guide.grpc.pb.h")
add_custom_command(
        OUTPUT "${hw_proto_srcs}" "${hw_proto_hdrs}" "${hw_grpc_srcs}" "${hw_grpc_hdrs}"
        COMMAND ${_PROTOBUF_PROTOC}
        ARGS --grpc_out "${CMAKE_CURRENT_BINARY_DIR}"
        --cpp_out "${CMAKE_CURRENT_BINARY_DIR}"
        -I "${hw_proto_path}"
        --plugin=protoc-gen-grpc="${_GRPC_CPP_PLUGIN_EXECUTABLE}"
        "${hw_proto}"
        DEPENDS "${hw_proto}")

# Include generated *.pb.h files
include_directories("${CMAKE_CURRENT_BINARY_DIR}")
message("TOTOTOTTOTO222 = __${hw_proto_srcs}__ __${hw_grpc_srcs}__ __${hw_proto}__")
message("atchoum ${CMAKE_CURRENT_BINARY_DIR}")

MESSAGE(CUICUICICICICI>>> ${hw_proto_srcs} ${hw_grpc_srcs})

message(TATATA __${_GRPC_GRPCPP_UNSECURE}__ __${_PROTOBUF_LIBPROTOBUF}__)
add_executable(route_guide_client "route_guide_client.cc" ${hw_proto_srcs} ${hw_grpc_srcs})

target_link_libraries(route_guide_client ${_GRPC_GRPCPP_UNSECURE} ${_PROTOBUF_LIBPROTOBUF} gRPC::grpc gRPC::grpc++_unsecure protobuf::libprotobuf)

add_custom_target(CopyGeneratedProtocolBufferFiles
        # todo: check if debug and release folder exist
        # debug version
#        COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_CURRENT_BINARY_DIR}/route_guide.grpc.pb.cc ${CMAKE_BINARY_DIR}/Debug

        )
add_dependencies(route_guide_client CopyGeneratedProtocolBufferFiles)

message(I will include ${CMAKE_CURRENT_BINARY_DIR})
include_directories(${CMAKE_CURRENT_BINARY_DIR})

if(EXISTS ${CMAKE_BINARY_DIR}/../src/network/grpc)
    add_custom_target(CopyGeneratedFiles
        COMMAND ${CMAKE_COMMAND} -E copy_if_different  ${CMAKE_BINARY_DIR}/protobuf/*.pb.h ${CMAKE_BINARY_DIR}/../src/network/grpc/.
        COMMAND ${CMAKE_COMMAND} -E copy_if_different  ${CMAKE_BINARY_DIR}/protobuf/*.pb.cc ${CMAKE_BINARY_DIR}/../src/network/grpc/.
    )
endif()

if(EXISTS ${CMAKE_BINARY_DIR}/../../src/network/grpc)
    add_custom_target(CopyGeneratedFiles
            COMMAND ${CMAKE_COMMAND} -E copy_if_different  ${CMAKE_BINARY_DIR}/protobuf/*.pb.h ${CMAKE_BINARY_DIR}/../../src/network/grpc/.
            COMMAND ${CMAKE_COMMAND} -E copy_if_different  ${CMAKE_BINARY_DIR}/protobuf/*.pb.cc ${CMAKE_BINARY_DIR}/../../src/network/grpc/.
            )
endif()

add_dependencies(CopyGeneratedFiles route_guide_client)

