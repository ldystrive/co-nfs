#pragma once

#include <cstdint>
#include <iostream>
#include <sys/inotify.h>

namespace inotify {

enum class Event : uint32_t {
    access = IN_ACCESS,
    modify = IN_MODIFY,
    attrib = IN_ATTRIB,
    close_write = IN_CLOSE_WRITE,
    close_nowrite = IN_CLOSE_NOWRITE,
    close = IN_CLOSE,
    open = IN_OPEN,
    moved_from = IN_MOVED_FROM,
    moved_to = IN_MOVED_TO,
    move = IN_MOVE,
    create = IN_CREATE,
    remove = IN_DELETE,
    remove_self = IN_DELETE_SELF,
    move_self = IN_MOVE_SELF,
    unmount = IN_UNMOUNT,
    q_overflow = IN_Q_OVERFLOW,
    ignored = IN_IGNORED,
    all_events = IN_ALL_EVENTS,
};
constexpr Event operator &(Event lhs, Event rhs)
{
    return static_cast<Event>(
        static_cast<std::underlying_type<Event>::type>(lhs)
        & static_cast<std::underlying_type<Event>::type>(rhs));
}

constexpr Event operator |(Event lhs, Event rhs)
{
    return static_cast<Event>(
        static_cast<std::underlying_type<Event>::type>(lhs)
        | static_cast<std::underlying_type<Event>::type>(rhs));
}
}