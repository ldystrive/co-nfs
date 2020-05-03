aux_source_directory(. CONFS_DIR_SRCS)

find_package(Threads)

add_executable(co-nfs ${CONFS_DIR_SRCS})
# find_library(zk_utils)
# find_library(inotify_utils)
target_link_libraries(co-nfs
    zk_utils
    inotify_utils
    Boost::system
    Boost::filesystem
    Boost::program_options
    ${CMAKE_THREAD_LIBS_INIT})