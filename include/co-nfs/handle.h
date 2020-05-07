#pragma once

#include <string>
#include <vector>

#include <boost/thread/thread.hpp>

#include "../json/json.hpp"
#include "../zookeeper/zk.h"
#include "../inotify/inotifyBuilder.h"

void addresses_cb(zhandle_t *zh, int type, int state, const char *path, void *ctx);

void events_cb(zhandle_t *zh, int type, int state, const char *path, void *ctx);

void ignore_cb(zhandle_t *zh, int type, int state, const char *path, void *ctx);

class Confs;

class EventHandler {

public:
    EventHandler();
    ~EventHandler();

    int handleCreate(std::string path, Confs *confs, inotify::Event event);
    int handleMove(std::string path_from, std::string path_to, Confs *confs);
    int handleCloseWrite(std::string path, Confs *confs, inotify::Event event);
    int handleRemove(std::string path, Confs *confs, inotify::Event event);

    int solveEvent(std::string eventId, std::string event, Confs *confs);
    int solveEvent(std::string eventId, nlohmann::json event, Confs *confs);
    int finishedEventId(std::string eventId);

private:
    volatile bool mRunning;
    boost::shared_mutex mMutex;
};
