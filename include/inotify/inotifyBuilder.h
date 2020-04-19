#pragma once

#include <inotify-cpp/Inotify.h>
#include "inotifyEvent.h"

#include <boost/filesystem.hpp>

#include <list>
#include <map>
#include <memory>
#include <utility>
#include <string>

namespace inotify {

using EventObserver = std::function<void(InotifyEvent)>;
using MoveObserver = std::function<void(InotifyEvent, InotifyEvent)>;

class InotifyBuilder {
public:
    InotifyBuilder();

    auto run() -> void;
    auto runOnce() -> void;
    auto stop() -> void;
    // 需要先调用onEvent[s]，再调用watch
    auto watchpathRecursively(boost::filesystem::path path) ->InotifyBuilder&;
    auto watchFile(boost::filesystem::path path) ->InotifyBuilder&;
    auto unwatchFile(boost::filesystem::path path) ->InotifyBuilder&;
    auto ignoreFileOnce(boost::filesystem::path path) ->InotifyBuilder&;
    auto ignoreFile(boost::filesystem::path path) ->InotifyBuilder&;
    auto onEvent(Event event, EventObserver) -> InotifyBuilder&;
    auto onEvents(std::vector<Event> event, EventObserver) -> InotifyBuilder&;
    auto onMoveEvent(MoveObserver) -> InotifyBuilder&;
    auto onUnexpectedEvent(EventObserver) -> InotifyBuilder&;

private:
    auto handleEvent(const InotifyEvent &inotifyEvent) -> void;
    auto addEvent(const InotifyEvent &InotifyEvent) -> void;

private:
    std::shared_ptr<Inotify> mInotify;
    std::map<Event, EventObserver> mEventObserver;
    MoveObserver mMoveObserver;
    std::queue<std::pair<InotifyEvent, std::chrono::steady_clock::time_point>> mEventQueue;
    EventObserver mUnexpectedEventObserver;
};

InotifyBuilder BuildInotify();

}