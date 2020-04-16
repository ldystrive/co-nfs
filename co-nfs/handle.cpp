#include <iostream>
#include <cstdio>
#include <utility>
#include <vector>
#include <string>

#include "zookeeper/zk.h"
#include "zookeeper/zkCallback.h"

using namespace std;

void init(const vector<string> &hosts, const string &localIp) {
    
    ZkUtils *zk = ZkUtils::GetInstance();
    zhandle_t *zh = zk->init_handle(zk_init_cb, hosts);
    assert(zh != NULL);
    zk->localIp = localIp;
    zk->createLayout();
    
    // zk->setNodeWatcher("/test", NULL);
    // zk->setChildrenWatcher("/test", NULL);
}