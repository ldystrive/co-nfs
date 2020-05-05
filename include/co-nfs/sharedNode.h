#pragma once

#include <iostream>
#include <cstdlib>
#include <cassert>
#include <cstdio>
#include <vector>
#include <string>
#include <utility>
#include <list>

#include <boost/optional.hpp>
#include <boost/thread/thread.hpp>

class SharedNode {
public:
    ~SharedNode();
    SharedNode();
    SharedNode(const std::string &name,
        const std::string &ignore,
        const std::vector<std::pair<std::string, std::string>> &addrs);
    
    // str likes: 192.168.137.132_1567464848020645536, ip_hash(mount path)
    static std::string parseAddr(const std::string &str);

public:
    std::string nodeName;
    std::string ignore;
    // pair<string, string>: ip & mount path
    std::vector<std::pair<std::string, std::string> > addresses;
    
};

class EventQueue {
public:
    ~EventQueue();
    EventQueue();
    
    boost::optional<std::pair<std::string, std::string>> front();
    void setQueue(const std::vector<std::pair<std::string, std::string>> &events);
    std::vector<std::pair<std::string, std::string>> getQueue();

private:
    void sortByName();
    // sorted by the first string
    // eventid, event
    boost::shared_mutex mMutex;
    std::vector<std::pair<std::string, std::string>> mEvents;
};