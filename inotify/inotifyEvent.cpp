#include "inotify/inotifyEvent.h"
#include <ctime>
#include <iostream>
#include <chrono>

#include "json/json.hpp"

using json = nlohmann::json;

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
    json j = json::parse(str);
    j.at("event").get_to(event);
    j.at("path").get_to(path);
}

InotifyEvent::InotifyEvent(const json &j)
{
    cookie = 0;
    j.at("event").get_to(event);
    j.at("path").get_to(path);
}

json InotifyEvent::toJson()
{
    json j;
    j["event"] = std::to_string(static_cast<uint32_t>(event));
    j["path"] = path.string();
    return j;
}

std::string InotifyEvent::toString()
{
    json &&j = toJson();
    return j.dump();
}

}