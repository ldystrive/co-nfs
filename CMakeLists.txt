project(co-nfs)

cmake_minimum_required(VERSION 3.8)

add_definitions(-std=c++11)

find_package(
    Boost 1.54.0
    COMPONENTS system filesystem
    REQUIRED
)

find_package(Threads)

include_directories(
    ${PROJECT_SOURCE_DIR}/include
    ${PROJECT_SOURCE_DIR}/include/inotify
    ${PROJECT_SOURCE_DIR}/include/nfs
    ${PROJECT_SOURCE_DIR}/include/zookeeper
    ${PROJECT_SOURCE_DIR}/include/co-nfs
    /usr/include
    /usr/local/include)

link_directories(
    ${PROJECT_BINARY_DIR}/lib
    /usr/lib
    /usr/local/lib)

set(EXECUTABLE_OUTPUT_PATH ${PROJECT_BINARY_DIR}/bin)
set(LIBRARY_OUTPUT_PATH ${PROJECT_BINARY_DIR}/lib)

add_subdirectory(third_party)
add_subdirectory(inotify)
add_subdirectory(zookeeper)
add_subdirectory(co-nfs)
