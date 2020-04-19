#include "inotify/inotifyEvent.h"
#include <ctime>
#include <iostream>
#include <chrono>

namespace inotify {

InotifyEvent::InotifyEvent(
    const Event &event,
    const uint32_t &cookie,
    const boost::filesystem::path &path)
    : event(event)
    , cookie(cookie)
    , path(path) {}

InotifyEvent::InotifyEvent(const std::string &str)
{
    cookie = 0;
    auto pos = str.find(";");
    if (pos == str.npos) {
        event = Event::all;
        path = boost::filesystem::path("");
    }
    else {
        event = static_cast<Event>(std::stoul(str.substr(0, pos - 1)));
        path = boost::filesystem::path(str.substr(pos, str.size()));
    }
}

std::string InotifyEvent::toString()
{
    std::string str = {};
    str = std::to_string(static_cast<uint32_t>(event));
    str = str + ";" + path.string();
    return str;
}

}