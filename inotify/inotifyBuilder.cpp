#include "inotify/inotifyBuilder.h"

namespace inotify {

InotifyBuilder::InotifyBuilder()
    : mInotify(std::make_shared<Inotify>()) {}

InotifyBuilder BuildInotify()
{
    return {};
}

auto InotifyBuilder::watchpathRecursively(boost::filesystem::path path) -> InotifyBuilder&
{
    mInotify->watchDirectoryRecursively(path);
    return *this;
}

auto InotifyBuilder::watchFile(boost::filesystem::path path) -> InotifyBuilder&
{
    mInotify->watchFile(path);
    return *this;
}

auto InotifyBuilder::unwatchFile(boost::filesystem::path path) -> InotifyBuilder&
{
    mInotify->unwatchFile(path);
    return *this;
}

auto InotifyBuilder::ignoreFileOnce(boost::filesystem::path path) -> InotifyBuilder&
{
    mInotify->ignoreFileOnce(path);
    return *this;
}

auto InotifyBuilder::ignoreFile(boost::filesystem::path path) -> InotifyBuilder&
{
    mInotify->ignoreFile(path);
    return *this;
}

auto InotifyBuilder::onEvent(Event event, EventObserver eventOberver) -> InotifyBuilder&
{
    mInotify->setEventMask(mInotify->getEventMask() | static_cast<std::uint32_t>(event));
    mEventObserver[event] = eventOberver;
    return *this;
}

auto InotifyBuilder::onEvents(std::vector<Event> events, EventObserver eventObserver) -> InotifyBuilder&
{
    for (auto event : events) {
        mInotify->setEventMask(mInotify->getEventMask() | static_cast<std::uint32_t>(event));
        mEventObserver[event] = eventObserver;
    }
    return *this;
}

auto InotifyBuilder::onMoveEvent(MoveObserver moveObserver) -> InotifyBuilder&
{
    mMoveObserver = moveObserver;
    return *this;
}

auto InotifyBuilder::onUnexpectedEvent(EventObserver eventObserver) -> InotifyBuilder&
{
    mUnexpectedEventObserver = eventObserver;
    return *this;
}

auto InotifyBuilder::runOnce() -> void
{
    auto fileSystemEvent = mInotify->getNextEvent();
    if (!fileSystemEvent) {
        return;
    }
    
    Event currentEvent = static_cast<Event>(fileSystemEvent->mask);
    InotifyEvent inotifyEvent {currentEvent, fileSystemEvent->cookie, fileSystemEvent->path};
    addEvent(inotifyEvent);
}

auto InotifyBuilder::run() -> void
{
    while (true) {
        if (mInotify->hasStopped()) {
            break;
        }
        runOnce();
    }
}

auto InotifyBuilder::stop() -> void
{
    mInotify->stop();
}

auto InotifyBuilder::addEvent(const InotifyEvent &inotifyEvent) -> void
{
    bool isEventUsed = false;
    std::chrono::steady_clock::time_point timeNow = std::chrono::steady_clock::now();
    std::chrono::milliseconds timeout(250);

    while (!mEventQueue.empty() || !isEventUsed) {
        // 最后处理inotifyEvent
        if (mEventQueue.empty()) {
            isEventUsed = true;
            mEventQueue.push({inotifyEvent, timeNow});
        }
        auto m = mEventQueue.front();

        // 一个独立的Event
        if (m.first.cookie == 0) {
            handleEvent(m.first);
            mEventQueue.pop();
        }
        // 绑定move_from与move_to
        else if (isEventUsed == false && m.first.cookie == inotifyEvent.cookie &&
            containsEvent(m.first.event, Event::moved_from) && 
            containsEvent(inotifyEvent.event, Event::moved_to)) {
            
            isEventUsed = true;
            removePath(m.first.path);
            addPath(inotifyEvent.path);
            mEventQueue.pop();
            mMoveObserver(m.first, inotifyEvent);
        }
        else if (std::chrono::duration_cast<std::chrono::milliseconds>(timeNow - m.second) < timeout) {
            break;
        }
        else {
            mUnexpectedEventObserver(m.first);
        }
    }

    // 前面还有目前无法处理的事件
    if (isEventUsed == false) {
        mEventQueue.push({inotifyEvent, std::chrono::steady_clock::now()});
    }
}

auto InotifyBuilder::handleEvent(const InotifyEvent &inotifyEvent) -> void
{
    

}

}