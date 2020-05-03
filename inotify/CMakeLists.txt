cmake_minimum_required(VERSION 3.8)

find_package(Threads)

set(INOTIFY_LIB_SRCS
    inotifyUtils.cpp
    inotifyBuilder.cpp
    inotifyEvent.cpp)

add_executable(inotify_example main.cpp)

add_library(inotify_utils ${INOTIFY_LIB_SRCS})

target_link_libraries(inotify_utils
    PRIVATE
    inotify-cpp
    Boost::system
    Boost::filesystem
    ${CMAKE_THREAD_LIBS_INIT})

target_link_libraries(inotify_example inotify_utils)