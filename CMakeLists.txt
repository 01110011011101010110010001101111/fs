cmake_minimum_required(VERSION 3.10)

project(SimpleFileSystem)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED True)

set(SOURCE_DIR src_backup)
# add_executable(Coordinator ${SOURCE_DIR}/Coordinator.cpp)
# add_executable(Server ${SOURCE_DIR}/Server.cpp ${SOURCE_DIR}/Filesystem.hpp)
add_executable(main   ${SOURCE_DIR}/main.cpp
                      ${SOURCE_DIR}/Client.cpp 
                      ${SOURCE_DIR}/Client.hpp 
                      ${SOURCE_DIR}/Server.hpp 
                      ${SOURCE_DIR}/Server.cpp 
                      ${SOURCE_DIR}/Coordinator.hpp 
                      ${SOURCE_DIR}/Coordinator.cpp
)
