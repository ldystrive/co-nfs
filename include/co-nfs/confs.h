#pragma once

#include <iostream>
#include <vector>
#include <string>
#include <thread>
#include <mutex>

#include <boost/thread/thread.hpp>
#include <boost/optional.hpp>

#include "sharedNode.h"
#include "../zookeeper/zk.h"
#include "../inotify/inotifyBuilder.h"

class Confs {
public:
    ~Confs();
    Confs(const std::vector<std::string> &hosts,
        const std::string &localPath,
        const std::string &nodeName);
    
    // 从zookeeper server上拉取结点信息
    boost::optional<SharedNode> pullNode();
    boost::optional<std::vector<pair<std::string, std::string>>> pullAddresses();
    boost::optional<std::string> pullIgnore();

    // 监控本地文件，并将修改事件发送到zookeeper server
    int watchLocalFiles();

    // 设置zookeeper watcher，监控server端变化
    int watchServerInfo();

public:
    std::thread inotifyThread;

public:
    SharedNode getNode();
    void setNode(SharedNode node);
    void updateNode();
    void updateAddresses();
    void updateIgnore();
private:
    SharedNode mNode;
    boost::shared_mutex mMutex;

private:
    Confs();
    ZkUtils *zk;
    EventQueue mEventQueue;
    inotify::InotifyBuilder notifier;
    boost::filesystem::path inFolder;
    boost::filesystem::path outFolder;
};