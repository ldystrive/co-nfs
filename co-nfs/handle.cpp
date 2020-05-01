#include <iostream>
#include <cstdio>
#include <utility>
#include <vector>
#include <string>

#include <boost/filesystem.hpp>

#include "zookeeper/zk.h"
#include "zookeeper/zkCallback.h"
#include "co-nfs/sharedNode.h"

using namespace std;

void init(const vector<string> &hosts, const string &localPath, const string &nodeName)
{
    
    ZkUtils *zk = ZkUtils::GetInstance();
    zhandle_t *zh = zk->init_handle(zk_init_cb, hosts);
    assert(zh != NULL);
    zk->nodeName = nodeName;
    assert(zk->initLocalPath(localPath) == 0);
    zk->createLayout();

    
    

    
    
    // zk->setNodeWatcher("/test", NULL);
    // zk->setChildrenWatcher("/test", NULL);
}