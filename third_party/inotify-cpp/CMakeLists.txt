set(INOTIFY_LIB_NAME inotify-cpp)
set(INOTIFY_LIB_SRCS
        src/NotifierBuilder.cpp
        src/Event.cpp
        src/FileSystemEvent.cpp
        src/Inotify.cpp
        src/Notification.cpp)
set(INOTIFY_LIB_HEADER
        include/inotify-cpp/NotifierBuilder.h
        include/inotify-cpp/Event.h
        include/inotify-cpp/FileSystemEvent.h
        include/inotify-cpp/Inotify.h
        include/inotify-cpp/Notification.h)

cmake_minimum_required(VERSION 3.8)


add_library(${INOTIFY_LIB_NAME} SHARED ${INOTIFY_LIB_SRCS} ${INOTIFY_LIB_HEADER})
set_target_properties(${INOTIFY_LIB_NAME} PROPERTIES LINKER_LANUAGE CXX)

target_compile_features(${INOTIFY_LIB_NAME} PUBLIC cxx_std_11)

target_link_libraries(${INOTIFY_LIB_NAME}
        INTERFACE
        Boost::system
        Boost::thread
        Boost::filesystem)

target_include_directories(${INOTIFY_LIB_NAME} PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
)