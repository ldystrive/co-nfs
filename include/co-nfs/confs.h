#pragma once

#include <iostream>
#include <memory>
#include <vector>
#include <string>
#include <thread>
#include <tuple>
#include <mutex>
#include <map>
#include <list>

#include <boost/thread/thread.hpp>
#include <boost/optional.hpp>

#include "handle.h"
#include "sharedNode.h"
#include "threadPool.h"
#include "fileTransfer.h"
#include "../json/json.hpp"
#include "../zookeeper/zk.h"
#include "../inotify/inotifyBuilder.h"

const size_t THREADS_NUM = 5;
// #define CHECK_NFS_MOUNT

class Confs {
public:
    ~Confs();
    Confs(const std::vector<std::string> &hosts,
        const std::string &localPath,
        const std::string &nodeName,
        const std::string &port);
    
    // 从zookeeper server上拉取结点信息
    boost::optional<SharedNode> pullNode();
    boost::optional<std::vector<std::tuple<std::string, std::string, std::string>>> pullAddresses();
    boost::optional<std::string> pullIgnore();
    boost::optional<std::vector<std::pair<std::string, std::string>>> pullEventQueue();

    bool isIgnoredPath(boost::filesystem::path path);

    // 监控本地文件，并将修改事件发送到zookeeper server
    int watchLocalFiles();

    // 设置zookeeper watcher，监控server端变化
    int watchServerInfo();

    void consistencyCheck();

    std::string convertPath(std::string rawPath, std::string baseDir);

    bool tryStartEvent(std::string id);
    void stopEvent();

    bool isLocalEvent(const std::string &event);
    bool isLocalEvent(const nlohmann::json &event);
    bool isLocalEvent(std::string ip, std::string port);

    bool checkLocalEvent(std::string path);
    bool checkLocalEvent(boost::filesystem::path path);
    void pushLocalEvent(std::string e);

    std::string getHandlingId();

public:
    std::thread inotifyThread;
    std::thread tcpServerThread;
    std::shared_ptr<ThreadPool> mPool;

public:
    ZkUtils *zk;
    EventQueue eventQueue;
    EventHandler eventHandler;
    boost::filesystem::path inFolder;
    boost::filesystem::path outFolder;
    inotify::InotifyBuilder notifier;
    std::list<std::pair<std::string, std::chrono::time_point<std::chrono::steady_clock>>> eventsTriggeredByThis;
    SharedNode getNode();
    void setNode(SharedNode node);
    void updateNode();
    void updateAddresses();
    void updateIgnore();
    void updateEventQueue();
    std::string getPort();

private:
    SharedNode mNode;
    std::atomic<bool> mHandlingEvent;
    std::string mHandlingId;
    std::string lastId;
    boost::shared_mutex mEventMutex;
    boost::shared_mutex mNodeMutex;
    boost::shared_mutex localEventsMutex;

private:
    Confs() = delete;
    Confs(const Confs&) = delete;
    Confs& operator=(const Confs&) = delete;
    bool isTriggeredByEventHandler(boost::filesystem::path path);
    const std::string port;
    std::shared_ptr<AsyncTcpServer> fileTransfer;
};
