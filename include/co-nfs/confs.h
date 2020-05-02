#pragma once

#include <iostream>
#include <vector>
#include <string>
#include <thread>

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
    boost::optional<SharedNode> getNode(std::string nodeName);

    // 监控本地文件，并将修改事件发送到zookeeper server
    int watchLocalFiles();

public:
    SharedNode node;

private:
    Confs();
    ZkUtils *zk;
    std::thread inotifyThread;
    inotify::InotifyBuilder notifier;
};