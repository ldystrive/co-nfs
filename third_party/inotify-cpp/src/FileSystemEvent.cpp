#include <inotify-cpp/FileSystemEvent.h>

#include <sys/inotify.h>

namespace inotify {
FileSystemEvent::FileSystemEvent(
    const int wd,
    uint32_t mask,
    uint32_t cookie,
    const boost::filesystem::path& path,
    const std::chrono::steady_clock::time_point& eventTime)
    : wd(wd)
    , mask(mask)
    , cookie(cookie)
    , path(path)
    , eventTime(eventTime)
{
}

FileSystemEvent::~FileSystemEvent()
{
}

const FileSystemEvent FileSystemEvent::voidEvent = {-1, 0, 0, boost::filesystem::path(""), std::chrono::steady_clock::now()};

bool isVoidEvent(const FileSystemEvent &fileSystemEvent)
{
    return fileSystemEvent.wd == -1 && fileSystemEvent.cookie == 0 && fileSystemEvent.mask == 0;
}

}