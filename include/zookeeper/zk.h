#ifndef __ZK_H_
#define __ZK_H_

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

class zkUtils {

public:

zhandle_t *zh;
vector<string> hosts;


int init_zoo(zhandle_t *zh, vector<string> hosts, watcher_fn fn);
int init_handle(watcher_fn fn);
int close_handle();
int createRecursively();


};

#endif