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

using namespace std;

class ZkUtils {

public:
    static ZkUtils *&GetInstance();
    static void deleteInstance();
    
public:

    zhandle_t *zh;
    vector<string> m_hosts;
    const static int timeout;

public:
    zhandle_t *init_handle(watcher_fn fn, const vector<string> &hosts);
    int close_handle();
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
