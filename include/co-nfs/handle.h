#pragma once
#include "../zookeeper/zk.h"

// 连接zookeeper的回调函数, 检查与构造zookeeper中的结构，再进行下一步的操作
void zk_init_cb(zhandle_t* zh, int type, int state, const char* path, void *watcherCtx);


