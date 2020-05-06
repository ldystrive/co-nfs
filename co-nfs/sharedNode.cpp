#include <iostream>
#include <algorithm>
#include <cstdlib>
#include <cstdio>
#include <vector>
#include <utility>
#include <string>

#include "co-nfs/sharedNode.h"

using namespace std;

SharedNode::~SharedNode() {}

SharedNode::SharedNode() {}

SharedNode::SharedNode(const string &name, const string &ignore, const vector<pair<string, string>> &addrs)
    : nodeName(name), ignore(ignore)
{
    addresses.clear();
    for (const auto &addrp : addrs) {
        string addr = addrp.first;
        string path = addrp.second;
        
        addr = parseAddr(addr);
        addresses.push_back({addr, path});
    }
}

string SharedNode::parseAddr(const string &str)
{
    auto pos = str.find("_");
    if (pos == str.npos) {
        return str;
    }
    return str.substr(0, pos);
}

EventQueue::EventQueue() {}
EventQueue::~EventQueue() {}

void EventQueue::sortByName()
{
    // boost::unique_lock<boost::shared_mutex> m(mMutex);
    sort(mEvents.begin(), mEvents.end(),
        [](const pair<string, string> &event1, const pair<string, string> &event2) -> bool
        {
            return event1.first < event2.first;
        });
}

void EventQueue::setQueue(const vector<pair<string, string>> &events)
{
    boost::unique_lock<boost::shared_mutex> m(mMutex);
    mEvents = move(events);
    sortByName();
}

boost::optional<pair<string, string>> EventQueue::front()
{
    boost::shared_lock<boost::shared_mutex> m(mMutex);
    if (mEvents.empty()) {
        return boost::none;
    }
    return mEvents[0];
}

vector<pair<string, string>> EventQueue::getQueue()
{
    boost::shared_lock<boost::shared_mutex> m(mMutex);
    return mEvents;
}