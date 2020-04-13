project(co-nfs)

cmake_minimum_required(VERSION 3.8)

add_definitions(-std=c++11)


add_subdirectory(third_party)
add_subdirectory(inotify)
add_subdirectory(zookeeper)