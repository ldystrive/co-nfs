#pragma once

#include <iostream>

#include <zookeeper/zookeeper.h>
#include "zk.h"

// 连接zookeeper的回调函数, 检查与构造zookeeper中的结构，再进行下一步的操作
void zk_init_cb(zhandle_t* zh, int type, int state, const char* path, void *watcherCtx);

// 通过future返回error code
void future_rc_completion_cb(int rc, const struct Stat *stat, const void *data);

// 通过future返回error code, string
void future_string_completion_cb(int rc, const char *value, const void *data);