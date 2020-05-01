find_library(ZOOKEEPER zookeeper_mt /usr/local/lib /usr/lib)

find_package(Threads)
set(ZOO_LIB_SRCS
    zk.cpp
    zkCallback.cpp)

# add_executable(zk_test main.cpp ${ZOO_LIB_SRCS})

add_library(zk_utils ${ZOO_LIB_SRCS})
target_link_libraries(zk_utils ${ZOOKEEPER} ${CMAKE_THREAD_LIBS_INIT})

# target_link_libraries(zk_test ${ZOOKEEPER} ${CMAKE_THREAD_LIBS_INIT})