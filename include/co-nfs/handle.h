#pragma once

#include <string>
#include <vector>

#include "../json/json.hpp"
#include "../zookeeper/zk.h"

void addresses_cb(zhandle_t *zh, int type, int state, const char *path, void *ctx);

void events_cb(zhandle_t *zh, int type, int state, const char *path, void *ctx);

void ignore_cb(zhandle_t *zh, int type, int state, const char *path, void *ctx);

class Confs;

class EventHandler {

public:
    EventHandler();
    ~EventHandler();

    int solveEvent(std::string eventId, std::string event, Confs *confs);
    int solveEvent(std::string eventId, nlohmann::json event, Confs *confs);
    int finishedEventId(std::string eventId);
};