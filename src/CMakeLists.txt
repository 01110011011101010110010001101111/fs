cmake_minimum_required (VERSION 3.10)

project (SimpleFileSystem)

set(PROJECT_SOURCE_DIR ${CMAKE_SOURCE_DIR})
set(CMAKE_BINARY_DIR ${CMAKE_SOURCE_DIR}/build)
set(EXECUTABLE_OUTPUT_PATH ${CMAKE_BINARY_DIR})
set(LIBRARY_OUTPUT_PATH ${CMAKE_BINARY_DIR})

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_BUILD_TYPE Release)

# find_library(IBVERBS_LIBRARY ibverbs)
# find_library(RDMACM_LIBRARY rdmacm)

# link_libraries(pthread ${IBVERBS_LIBRARY} ${RDMACM_LIBRARY})

# include_directories("${PROJECT_SOURCE_DIR}")

# add_executable(rdma_server ${PROJECT_SOURCE_DIR}/rdma_server.cpp)
# add_executable(rdma_client ${PROJECT_SOURCE_DIR}/rdma_client.cpp)

find_package(Protobuf REQUIRED)
find_package(gRPC REQUIRED)
find_package(PkgConfig REQUIRED)
find_package(absl CONFIG REQUIRED)
find_package(utf8_range CONFIG REQUIRED)

pkg_check_modules(FUSE3 REQUIRED fuse3)
message(STATUS "Using Protobuf ${Protobuf_VERSION}, gRPC ${gRPC_VERSION}")


set(PROTO_FILE ${CMAKE_CURRENT_SOURCE_DIR}/Rdmafs_RPC.proto)

# Target names for generated files
set(GENERATED_SRCS
    ${CMAKE_CURRENT_BINARY_DIR}/Rdmafs_RPC.pb.cc
    ${CMAKE_CURRENT_BINARY_DIR}/Rdmafs_RPC.grpc.pb.cc)
set(GENERATED_HDRS
    ${CMAKE_CURRENT_BINARY_DIR}/Rdmafs_RPC.pb.h
    ${CMAKE_CURRENT_BINARY_DIR}/Rdmafs_RPC.grpc.pb.h)

add_custom_command(
OUTPUT  ${GENERATED_SRCS} ${GENERATED_HDRS}
COMMAND protobuf::protoc
        ARGS  --grpc_out=${CMAKE_CURRENT_BINARY_DIR}
                --cpp_out=${CMAKE_CURRENT_BINARY_DIR}
                -I ${CMAKE_CURRENT_SOURCE_DIR}
                --plugin=protoc-gen-grpc=$<TARGET_FILE:gRPC::grpc_cpp_plugin>
                ${PROTO_FILE}
DEPENDS ${PROTO_FILE}
COMMENT "Generating gRPC & Protobuf sources from ${PROTO_FILE}"
VERBATIM)

add_library(rdmafs_proto STATIC
        ${GENERATED_SRCS} ${GENERATED_HDRS})
target_link_libraries(rdmafs_proto
  PUBLIC gRPC::grpc++ protobuf::libprotobuf utf8_range::utf8_range utf8_range::utf8_validity)

target_include_directories(rdmafs_proto
  PUBLIC ${CMAKE_CURRENT_BINARY_DIR})

add_executable(rdmafs_coordinator Rdmafs_Coordinator.cpp)
target_link_libraries(rdmafs_coordinator
PRIVATE rdmafs_proto gRPC::grpc++ protobuf::libprotobuf utf8_range::utf8_range utf8_range::utf8_validity pthread ${FUSE3_LIBRARIES} absl::cord absl::strings)
target_include_directories(rdmafs_coordinator
PRIVATE ${FUSE3_INCLUDE_DIRS})

add_custom_target(generate_proto DEPENDS ${GENERATED_SRCS})

add_executable(rdmafs Rdmafs.cpp)
target_link_libraries(rdmafs
PRIVATE rdmafs_proto gRPC::grpc++ pthread ${FUSE3_LIBRARIES})
target_include_directories(rdmafs
PRIVATE ${FUSE3_INCLUDE_DIRS})