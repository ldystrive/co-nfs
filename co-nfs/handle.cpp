#include <iostream>
#include <fstream>
#include <cstdio>
#include <utility>
#include <vector>
#include <string>

#include <boost/filesystem.hpp>
#include <boost/asio.hpp>

#include "zookeeper/zk.h"
#include "zookeeper/zkCallback.h"
#include "co-nfs/confs.h"
#include "co-nfs/handle.h"
#include "co-nfs/utils.h"
#include "co-nfs/fileTransfer.h"
#include "co-nfs/sharedNode.h"
#include "json/json.hpp"
#include "inotify/inotifyBuilder.h"

using namespace std;
using namespace inotify;
using json = nlohmann::json;
namespace fs = boost::filesystem;

void addresses_cb(zhandle_t *zh, int type, int state, const char *path, void *ctx)
{
    cout << __PRETTY_FUNCTION__ << endl;
    cout << "type:" << type << " state:" << state << " path:" << path << endl;
    zoo_awget_children(zh, path, addresses_cb, ctx, future_strings_completion_cb, NULL);
    
    Confs *confs = static_cast<Confs *>(ctx);

    confs->mPool->enqueue([confs](){
        confs->updateAddresses();
        SharedNode node = confs->getNode();
        cout << "new addresses:" << endl;
        for (auto a : node.addresses) {
            cout << get<0>(a) << ' ' << get<1>(a) << ' ' << get<2>(a) << endl;
        }
    });
}

void events_cb(zhandle_t *zh, int type, int state, const char *path, void *ctx)
{
    cout << __PRETTY_FUNCTION__ << endl;
    cout << "type:" << type << " state:" << state << " path:" << path << endl;
    zoo_awget_children(zh, path, events_cb, ctx, future_strings_completion_cb, NULL);

    Confs *confs = static_cast<Confs *>(ctx);

    confs->mPool->enqueue([confs](){
        confs->updateEventQueue();
        auto value = confs->eventQueue.getQueue();
        cout << "new event queue:" << endl;
        for (auto a : value) {
            cout << a.first << ' ' << a.second << endl;
        }
        if (!value.empty()){
//            int eventState = confs->getEventState();
//            if ()

            confs->eventHandler.solveEvent(value[0].first, value[0].second, confs);
        }
    });

}

void ignore_cb(zhandle_t *zh, int type, int state, const char *path, void *ctx)
{
    cout << __PRETTY_FUNCTION__ << endl;
    cout << "type:" << type << " state:" << state << " path:" << path << endl;

    zoo_awexists(zh, path, ignore_cb, ctx, future_rc_completion_cb, NULL);

    Confs *confs = static_cast<Confs *>(ctx);
    confs->mPool->enqueue([confs](){
        confs->updateIgnore();
        SharedNode node = confs->getNode();
        cout << "new ignore:" << node.ignore << endl;
    });
}

EventHandler::EventHandler() {}

EventHandler::~EventHandler() {}

int EventHandler::solveEvent(string eventId, string event, Confs *confs)
{
    solveEvent(eventId, json::parse(event), confs);
}

int EventHandler::handleCreate(string path, Confs *confs, Event event)
{
    if (fs::exists(path)) {
        cout << "path already exists:" << path  << '\n';
        return -1;
    }

    try {
        if ((event & Event::is_dir) != Event::none) {
            // create directory
            fs::create_directories(path);
        }
        else {
            // create file
            fs::create_directories(fs::path(path).parent_path());
            ofstream fileStream(path);
            fileStream.close();
        }
    }
    catch(const exception &e) {
        cout << e.what() << '\n';
        return -1;
    }
    
    return 0;
}

int EventHandler::handleCloseWrite(string path, Confs *confs, Event event)
{
    auto handleFinished = [&](string addr) {
        cout << "file transfer " << addr << "finished." << endl;
    };
    SharedNode node = confs->getNode();
    auto addrs = node.addresses;
    for (const auto &addr : addrs) {
        string ip = get<0>(addr);
        string port = get<1>(addr);
        string dir = get<2>(addr);
        string path_to = dir + path.substr(confs->zk->localDir.size(), path.size());
        if (ip == confs->zk->localIp && port == confs->getPort()) {
            continue;
        }
        try {
            boost::asio::io_service io_service;
            AsyncTcpClient client(io_service, ip, port, path, path_to, handleFinished);
            io_service.run();
        }
        catch (const exception &e) {
            cerr << "Exception in " << __PRETTY_FUNCTION__ << ": " << e.what() << "\n";
        }
    }
}

int EventHandler::handleRemove(string path, Confs *confs, Event event)
{
    if (!fs::exists(path)) {
        cout << "path does not exists:" << path << '\n';
        return -1;
    }

    try {
        fs::remove_all(path);
    }
    catch(const exception &e) {
        cout << e.what() << '\n';
        return -1;
    }
    return 0;
}

int EventHandler::handleMove(string path_from, string path_to, Confs *confs)
{
    if (!fs::exists(path_from)) {
        cout << "src path: " << path_from << " does not exist." << "\n";
        return -1;
    }
    if (fs::is_directory(path_from)) {
        try {
            mutils::copyDir(path_from, path_to);
            fs::remove_all(path_from);
        }
        catch (const exception &e) {
            cout << e.what() << '\n';
            return -1;
        }
    }
    else if(fs::is_regular_file(path_from)){
        try {
            fs::copy_file(path_from, path_to, fs::copy_option::overwrite_if_exists);
            fs::remove(path_from);
        }
        catch (const exception &e) {
            cout << e.what() << '\n';
            return -1;
        }
    }
    return 0;
}

int EventHandler::solveEvent(string eventId, json event, Confs *confs)
{
    string ip = event["ip"];
    string mountPath = event["path"];
    string port = event["port"];
    if (ip == confs->zk->localIp && port == confs->getPort()) {
        if (event.contains("event")) {
            string path = event["event"]["path"];
            Event e = static_cast<Event>(static_cast<uint32_t>(stoul(string(event["event"]["event"]))));
            if ((e & Event::close_write) != Event::none) {
                return handleCloseWrite(path, confs, e);
            }
        }
        return 0;
    }
    int res = 0;
    
    if (event.contains("event1") && event.contains("event2")) {
        string path_from = confs->convertPath(event["event1"]["path"], mountPath);
        string path_to = confs->convertPath(event["event2"]["path"], mountPath);
        res = handleMove(path_from, path_to, confs);

    }
    else if (event.contains("event")) {
        string path = confs->convertPath(event["event"]["path"], mountPath);

        Event e = static_cast<Event>(static_cast<uint32_t>(stoul(string(event["event"]["event"]))));
        if ((e & Event::create) != Event::none) {
            res = handleCreate(path, confs, e);
        }
        else if ((e & Event::remove) != Event::none) {
            res = handleRemove(path, confs, e);
        }
    }
    return res;
}
