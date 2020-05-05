#pragma once

#include <iostream>
#include <vector>
#include <string>
#include <thread>
#include <mutex>

#include <boost/thread/thread.hpp>
#include <boost/optional.hpp>

#include "sharedNode.h"
#include "threadPool.h"
#include "../zookeeper/zk.h"
#include "../inotify/inotifyBuilder.h"

const size_t THREADS_NUM = 4;
// #define CHECK_NFS_MOUNT

class Confs {
public:
    ~Confs();
    Confs(const std::vector<std::string> &hosts,
        const std::string &localPath,
        const std::string &nodeName);
    
    // 从zookeeper server上拉取结点信息
    boost::optional<SharedNode> pullNode();
    boost::optional<std::vector<std::pair<std::string, std::string>>> pullAddresses();
    boost::optional<std::string> pullIgnore();
    boost::optional<std::string> pullEventQueue();

    // 监控本地文件，并将修改事件发送到zookeeper server
    int watchLocalFiles();

    // 设置zookeeper watcher，监控server端变化
    int watchServerInfo();

    void consistencyCheck();

public:
    std::thread inotifyThread;
    std::shared_ptr<ThreadPool> mPool;

public:
    SharedNode getNode();
    void setNode(SharedNode node);
    void updateNode();
    void updateAddresses();
    void updateIgnore();

private:
    SharedNode mNode;
    boost::shared_mutex mNodeMutex;

private:
    Confs() = delete;
    Confs(const Confs&) = delete;
    Confs& operator=(const Confs&) = delete;
    ZkUtils *zk;
    EventQueue mEventQueue;
    inotify::InotifyBuilder notifier;
    boost::filesystem::path inFolder;
    boost::filesystem::path outFolder;
};