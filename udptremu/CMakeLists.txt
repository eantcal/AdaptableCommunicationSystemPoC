cmake_minimum_required(VERSION 3.1.0)
project(udptremu)

set(THREADS_PREFER_PTHREAD_FLAG ON)
find_package(Threads REQUIRED)

include_directories(SYSTEM)
include_directories(${CMAKE_CURRENT_SOURCE_DIR})
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/include)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/include/net)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/include/net/socket)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/include/utils)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/include/config)

FILE(GLOB SOURCES *.cc utils/*.cc net/*.cc net/socket/*.cc config/*.cc)

add_executable(udptremu ${SOURCES})

set_property(TARGET udptremu PROPERTY CXX_STANDARD 17)
set_property(TARGET udptremu PROPERTY CXX_STANDARD_REQUIRED ON)

set(CMAKE_BUILD_TYPE Debug)

target_link_libraries(udptremu PRIVATE Threads::Threads)

