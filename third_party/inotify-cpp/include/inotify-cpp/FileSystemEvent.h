#pragma once
#include <boost/filesystem.hpp>

#include <chrono>
#include <string>

namespace inotify {
class FileSystemEvent {
  public:
    FileSystemEvent(
        int wd,
        uint32_t mask,
        uint32_t cookie,
        const boost::filesystem::path& path,
        const std::chrono::steady_clock::time_point& eventTime);

    ~FileSystemEvent();

    const static FileSystemEvent voidEvent;

  public:
    int wd;
    uint32_t mask;
    uint32_t cookie;
    boost::filesystem::path path;
    std::chrono::steady_clock::time_point eventTime;
};


bool isVoidEvent(const FileSystemEvent &fileSystemEvent);

}