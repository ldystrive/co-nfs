find_library(ZOOKEEPER zookeeper_mt /usr/local/lib /usr/lib)

find_package(Threads)

add_executable(zk_test main.cpp)

target_link_libraries(zk_test ${ZOOKEEPER} ${CMAKE_THREAD_LIBS_INIT})