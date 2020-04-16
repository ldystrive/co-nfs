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

#define _DEV

using namespace std;

class ZkUtils {

public:
    static ZkUtils *&GetInstance();
    static void deleteInstance();
    
public:

    zhandle_t *zh;
    vector<string> m_hosts;
    const static int timeout;
    string localIp;

public:
    zhandle_t *init_handle(watcher_fn fn, const vector<string> &hosts);
    int close_handle();
    
    // 检查zookeeper server中的结构，如果没有初始化则初始化
    void createLayout();

    // 同步检测path是否存在
    int exists(string path);
    int create(string path, string value);
    int checkAndCreate(string path, string value);

    pair<int, vector<string> > ls(const string &path);

    void createSharedNode(string nodeName, const vector<pair<string, string> > &addresses, const vector<string> &ignore);
    
    // 拉取和该ip有关的信息
    void getNodesInfo();
    int createRecursively();


private:
    ZkUtils();
    ~ZkUtils();
    ZkUtils(const ZkUtils&);
    const ZkUtils &operator=(const ZkUtils &);

private:
    static ZkUtils *m_sInstance;
    static std::mutex m_mux;

};
