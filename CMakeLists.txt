cmake_minimum_required(VERSION 3.10)

# Set the project name
project(SimpleDistributedFileSystem)

# Set the C++ standard
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED True)

# Specify the source directory
set(SOURCE_DIR src)

# Add the executable for the main application
add_executable(SimpleDistributedFileSystem ${SOURCE_DIR}/main.cpp ${SOURCE_DIR}/Client.cpp ${SOURCE_DIR}/MetadataServer.cpp)
