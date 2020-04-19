#pragma once

#include <inotify-cpp/Event.h>
#include <inotify-cpp/FileSystemEvent.h>

#include <boost/filesystem.hpp>

#include <chrono>

namespace inotify {

class InotifyEvent {

public:
    InotifyEvent(
        const Event &event,
        const uint32_t &cookie,
        const boost::filesystem::path &path);
    
    // parse string to InotifyEvent
    InotifyEvent(const std::string &str);
    std::string toString();

public:
    Event event;
    uint32_t cookie;
    boost::filesystem::path path;
};

}