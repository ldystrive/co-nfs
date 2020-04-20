#include "inotify/inotifyBuilder.h"

#include <exception>

namespace inotify {

InotifyBuilder::InotifyBuilder()
    : mInotify(std::make_shared<Inotify>()) {}

InotifyBuilder BuildInotify()
{
    return {};
}

auto InotifyBuilder::watchpathRecursively(boost::filesystem::path path) -> InotifyBuilder&
{
    try {
        mInotify->watchDirectoryRecursively(path);
    }
    catch(const std::exception& e) {
        std::cerr << __PRETTY_FUNCTION__ << ' ';
        std::cerr << e.what() << '\n';
    }
    
    return *this;
}

auto InotifyBuilder::watchFile(boost::filesystem::path path) -> InotifyBuilder&
{
    try {
        mInotify->watchFile(path);
    }
    catch(const std::exception &e) {
        std::cerr << __PRETTY_FUNCTION__ << ' ';
        std::cout << e.what() << "\n";
    }
    return *this;
}

auto InotifyBuilder::unwatchFile(boost::filesystem::path path) -> InotifyBuilder&
{
    try {
        mInotify->unwatchFile(path);
    }
    catch(const std::exception& e) {
        std::cerr << __PRETTY_FUNCTION__ << ' ';
        std::cerr << e.what() << '\n';
    }
    return *this;
}

auto InotifyBuilder::ignoreFileOnce(boost::filesystem::path path) -> InotifyBuilder&
{
    try {
        mInotify->ignoreFileOnce(path);
    }
    catch(const std::exception& e) {
        std::cerr << __PRETTY_FUNCTION__ << ' ';
        std::cerr << e.what() << '\n';
    }
    
    return *this;
}

auto InotifyBuilder::ignoreFile(boost::filesystem::path path) -> InotifyBuilder&
{
    try {
        mInotify->ignoreFile(path);
    }
    catch(const std::exception& e) {
        std::cerr << __PRETTY_FUNCTION__ << ' ';
        std::cerr << e.what() << '\n';
    }
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
    if (inotifyEvent.event == Event::none) {
        isEventUsed = true;
    }
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
            if (containsEvent(m.first.event, Event::is_dir)) {
                // unwatchFile(m.first.path);
                // unwatch by ON_IGNORED event
            }
            if (containsEvent(inotifyEvent.event, Event::is_dir)) {
                watchFile(inotifyEvent.path);
            }
            mEventQueue.pop();
            mMoveObserver(m.first, inotifyEvent);
        }
        // 稍后处理
        else if (std::chrono::duration_cast<std::chrono::milliseconds>(timeNow - m.second) < timeout) {
            break;
        }
        // 超时， 从监控文件夹移出，相当于一个remove操作
        else if (containsEvent(m.first.event, Event::moved_from)) {
            Event newEvent = Event::remove;
            if (containsEvent(m.first.event, Event::is_dir)) {
                newEvent = newEvent | Event::is_dir;
            }
            InotifyEvent newInotifyEvent = {newEvent, m.first.cookie, m.first.path};
            handleEvent(newInotifyEvent);
            mEventQueue.pop();
        }
        // 超时，移动到监控目录，相当于create操作
        else if (containsEvent(m.first.event, Event::moved_to)) {
            Event newEvent = Event::create;
            if (containsEvent(m.first.event, Event::is_dir)) {
                newEvent = newEvent | Event::is_dir;
            }
            InotifyEvent newInotifyEvent = {newEvent, m.first.cookie, m.first.path};
            handleEvent(newInotifyEvent);
            mEventQueue.pop();
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
    auto eventAndEventObserver = mEventObserver.find(inotifyEvent.event);
    if (eventAndEventObserver != mEventObserver.end()) {
        auto &event = eventAndEventObserver->first;
        auto &eventObserver = eventAndEventObserver->second;

        if (containsEvent(event, Event::is_dir)) {
            if (containsEvent(event, Event::create)) {
                watchFile(inotifyEvent.path);
            }
            else if (containsEvent(event, Event::remove)) {
                // unwatchFile(inotifyEvent.path);
                // unwatch by ON_IGNORED event
            }
        }
        eventObserver(inotifyEvent);
    }
    else {
        mUnexpectedEventObserver(inotifyEvent);
    }
}

}