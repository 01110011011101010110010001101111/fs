cmake_minimum_required(VERSION 3.10)

project(SimpleFileSystem)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED True)

set(SOURCE_DIR src)
add_executable(SimpleFileSystem ${SOURCE_DIR}/fs.cc)
add_executable(Benchmark ${SOURCE_DIR}/benchmark.cc ${SOURCE_DIR}/FileSystem.h)
