#pragma once

#include <iostream>
#include <cstdlib>
#include <cassert>
#include <cstdio>
#include <vector>
#include <string>
#include <utility>
#include <list>
#include <tuple>

#include <boost/optional.hpp>
#include <boost/thread/thread.hpp>

class SharedNode {
public:
    ~SharedNode();
    SharedNode();
    SharedNode(const std::string &name,
        const std::string &ignore,
        const std::vector<std::tuple<std::string, std::string, std::string>> &addrs);
    
    static std::pair<std::string, std::string> parseAddr(const std::string &str);

public:
    std::string nodeName;
    std::string ignore;
    // tuple<string, string, string>: ip port and mount path
    std::vector<std::tuple<std::string, std::string, std::string> > addresses;
    
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

enum class EventState : uint32_t {
    waiting,
    ready,
    runningByMaster,
    runningBySlave,
    finished
};

enum class RunnningState : uint32_t {
    ready,
    running,
    finished
};
