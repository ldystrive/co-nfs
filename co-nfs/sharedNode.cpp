#include <iostream>
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

boost::optional<string> EventQueue::getNextEvent()
{
    if (empty()) {
        return boost::none;
    }
    return mEvents.front().second;
}

bool EventQueue::empty() 
{
    return mEvents.empty();
}

void EventQueue::insertEvent(const pair<string, string> &event)
{
    auto iter = mEvents.begin();
    while (iter != mEvents.end() && (*iter).first < event.first) {
        iter++;
    }
    mEvents.insert(iter, event);
}

void EventQueue::pop()
{
    mEvents.pop_front();
}