cmake_minimum_required(VERSION 3.8)

find_package(
    Boost 1.54.0
    COMPONENTS system filesystem
    REQUIRED
)

find_package(Threads)


add_executable(inotify_example main.cpp)

add_library(inotify_utils inotifyUtils.cpp)

target_link_libraries(inotify_example
    PRIVATE
    inotify-cpp
    Boost::system
    Boost::filesystem
    ${CMAKE_THREAD_LIBS_INIT})   

target_link_libraries(inotify_utils
    PRIVATE
    inotify-cpp
    Boost::system
    Boost::filesystem
    ${CMAKE_THREAD_LIBS_INIT})