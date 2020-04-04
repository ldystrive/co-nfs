project(nfstest)

cmake_minimum_required(VERSION 3.5)

aux_source_directory(. DIR_SRCS)

include_directories(/usr/local/include)
link_directories(/usr/local/lib)

add_executable( main ${DIR_SRCS})
target_link_libraries( main -lnfs)