#pragma once

#include <zookeeper/zookeeper.h>
#include <iostream>
#include <cstdlib>
#include <cassert>
#include <cstdio>
#include <thread>
#include <condition_variable>
#include <mutex>
#include <chrono>
#include <vector>
#include <string>

// #define _DEV


class ZkUtils {

public:
    static ZkUtils *&GetInstance();
    static void deleteInstance();
    
public:

    zhandle_t *zh;
    std::vector<std::string> m_hosts;
    const static int timeout;
    std::string localIp;
    std::string localDir;
    std::string nodeName;

public:
    zhandle_t *init_handle(watcher_fn fn, const std::vector<std::string> &hosts);
    int close_handle();

    int initLocalPath(const std::string& str);
    
    // 检查zookeeper server中的结构，如果没有初始化则初始化
    void createLayout();

    // 同步检测path是否存在
    int exists(std::string path);
    int create(std::string path, std::string value, const int mode=ZOO_PERSISTENT);
    int checkAndCreate(std::string path, std::string value, const int mode=ZOO_PERSISTENT);

    int remove(std::string path);
    int removeRecursively(std::string path);

    void setNodeWatcher(const std::string &path, void *ctx);
    void setChildrenWatcher(const std::string &path, void *ctx);

    std::pair<int, std::vector<std::string> > ls(const std::string &path);
    std::pair<int, std::vector<std::pair<std::string, std::string>>> ls2(const std::string &path);
    std::pair<int, std::string> get(const std::string &path);

    void createSharedNode(std::string nodeName,
        const std::vector<std::pair<std::string, std::string> > &addresses,
        const std::vector<std::string> &ignore);
    
    // 拉取和该ip有关的信息
    void getNodesInfo();
    int createRecursively();

    std::string getNodePath();

private:
    ZkUtils();
    ~ZkUtils();
    ZkUtils(const ZkUtils&);
    const ZkUtils &operator=(const ZkUtils &);

private:
    static ZkUtils *m_sInstance;
    static std::mutex m_mux;

};
