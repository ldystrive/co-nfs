#include "zookeeper/zk.h"
#include "zookeeper/zkCallback.h"

void init(const vector<string> &hosts, const string &localIp) {
    
    ZkUtils *zk = ZkUtils::GetInstance();
    zhandle_t *zh = zk->init_handle(zk_init_cb, hosts);
    assert(zh != NULL);
    zk->localIp = localIp;
    // zk->createLayout();
    
}