#pragma once

#include <inotify-cpp/Event.h>
#include <inotify-cpp/FileSystemEvent.h>

#include <boost/filesystem.hpp>

#include <chrono>

namespace inotify {

class Notification {
  public:
    Notification(
        const Event& event,
        const FileSystemEvent& fileSystemEvent,
        const boost::filesystem::path& path,
        std::chrono::steady_clock::time_point time);

  public:
    const Event event;
    const FileSystemEvent fileSystemEvent;
    const boost::filesystem::path path;
    const std::chrono::steady_clock::time_point time;
};
}