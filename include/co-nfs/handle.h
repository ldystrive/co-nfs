#pragma once

#include <string>
#include <vector>

#include "../zookeeper/zk.h"

void addresses_cb(zhandle_t *zh, int type, int state, const char *path, void *ctx);

void events_cb(zhandle_t *zh, int type, int state, const char *path, void *ctx);

void ignore_cb(zhandle_t *zh, int type, int state, const char *path, void *ctx);