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

#define RESET   "\033[0m"
#define RED     "\033[31m"      /* Red */
#define GREEN   "\033[32m"      /* Green */

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

void EventHandler::checkEventFinished(Confs *confs)
{
    int pullTimes = 50;
    int timeout = 200;
    auto id = confs->getHandlingId();
    while (--pullTimes) {
        cout << GREEN  << pullTimes  << statePath << RESET << endl;
        auto v = confs->zk->ls(statePath);
        cout << RED << v.first << " " << v.second.size() << RESET << endl;
        if (v.second.size() == 0) {
            break;
        }
        this_thread::sleep_for(chrono::milliseconds(timeout));
    }
    string eventsPath = confs->zk->getNodePath() + "/events/" + id;
    cout << "Remove: " << eventsPath << endl;
    confs->zk->removeRecursively(statePath);
    confs->zk->remove(eventsPath);
    //while (confs->zk->exists(eventsPath) != ZNONODE) {}
    if (!pullTimes) {
        cout << "Timeout. not all clients finished" << endl;
    }
}

void EventHandler::receivedFile(Confs *confs)
{
    boost::unique_lock<boost::shared_mutex> m(downloadReceivedMutex);
    while (confs->getHandlingId() == "") {}
    string path = confs->zk->getNodePath() + "/eventState/" + confs->getHandlingId();
    string subPath = path + "/" + confs->zk->localIp + "_" + confs->getPort();
    cout << GREEN << "received file" << " state path:" << subPath << " " << confs->zk->exists(subPath) << RESET << endl;
    // this_thread::sleep_for(chrono::minutes(600));
    cout << RED << "begin " << confs->zk->exists(subPath) << RESET << endl;
    while (confs->zk->exists(subPath) != ZOK) { 
        cout << RED << confs->zk->exists(subPath) << RESET << endl;
        this_thread::sleep_for(chrono::milliseconds(200));
    }
    cout << RED << "end " << confs->zk->exists(subPath) << RESET << endl;
    confs->zk->remove(subPath);
    confs->stopEvent();
}

void events_cb(zhandle_t *zh, int type, int state, const char *path, void *ctx)
{
    cout << __PRETTY_FUNCTION__ << endl;
    cout << "type:" << type << " state:" << state << " path:" << path << endl;
    zoo_awget_children(zh, path, events_cb, ctx, future_strings_completion_cb, NULL);

    Confs *confs = static_cast<Confs *>(ctx);
    boost::shared_mutex &downloadMutex = confs->eventHandler.downloadMutex;

    confs->mPool->enqueue([confs, &downloadMutex](){
        boost::unique_lock<boost::shared_mutex> m(downloadMutex);
        confs->updateEventQueue();
        auto value = confs->eventQueue.getQueue();
        cout << "new event queue:" << endl;
        for (auto a : value) {
            cout << a.first << ' ' << a.second << endl;
        }
        if (!value.empty() && confs->tryStartEvent(value[0].first)){
            cout << RED << "Start to solve event:" << value[0].first  << RESET << endl;
            string path = confs->zk->getNodePath() + "/eventState/" + value[0].first;
            confs->eventHandler.statePath = path;
            int64_t ttl = 2 * 60 * 1000;
            json event = json::parse(value[0].second);
            if (confs->isLocalEvent(event)) {
                string v = "";
                cout << RED << "path:" << path << RESET << endl;
                zoo_acreate(confs->zk->zh, path.c_str(), v.c_str(), v.length(),
                    &ZOO_OPEN_ACL_UNSAFE, ZOO_PERSISTENT, future_string_completion_cb, NULL);
                auto node = confs->getNode();
                for (const auto &addr : node.addresses) {
                    string ip = get<0>(addr);
                    string port = get<1>(addr);
                    if (confs->isLocalEvent(ip, port)) continue;
                    string subPath = path + "/" + ip + "_" + port;
                    cout << RED << "subPath:" << subPath << RESET << endl;
                    zoo_acreate(confs->zk->zh, subPath.c_str(), "", 0,
                        &ZOO_OPEN_ACL_UNSAFE, ZOO_EPHEMERAL, future_string_completion_cb, NULL);
                }
            }
            confs->eventHandler.solveEvent(value[0].first, json::parse(value[0].second), confs);
            if (confs->isLocalEvent(event)) {
                cout << RED << "check" << RESET << endl;
                confs->eventHandler.checkEventFinished(confs);
                cout << RED << "check finished" << RESET << endl;
                confs->stopEvent();
            }
            else if (event.contains("event")) {
                Event e = static_cast<Event>(static_cast<uint32_t>(stoul(string(event["event"]["event"]))));
                if ((e & Event::close_write) == Event::none) {
                    confs->eventHandler.receivedFile(confs);
                }
            }
            else {
                confs->eventHandler.receivedFile(confs);
            }
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
            // fs::create_directories(fs::path(path).parent_path());
            confs->notifier.ignoreFileOnce(fs::path(path));
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
    auto handleFinished = [=]() {
        /*
        auto node = confs->eventQueue.front();
        if (node) {
            string eventId = node->first;
            string statePath = confs->zk->getNodePath() + "/eventState/" + confs->zk->localIp + "_" + confs->zk->localDir;
            confs->zk->remove(statePath);
        }
        */
    };
    SharedNode node = confs->getNode();
    int nodeNum = node.addresses.size();

    vector<promise<int> *> promises(nodeNum);
    vector<future<int> > futures(nodeNum);

    for (int i = 0; i < nodeNum; i++) {
        promises[i] = new promise<int>();
        futures[i] = promises[i]->get_future();
    }
    vector<boost::asio::io_service> io_service(nodeNum);
    for (int i = 0; i < nodeNum; i++) {
        string ip = get<0>(node.addresses[i]);
        string port = get<1>(node.addresses[i]);
        string dir = get<2>(node.addresses[i]);
        string path_to = dir + path.substr(confs->zk->localDir.size(), path.size());
        if (ip == confs->zk->localIp && port == confs->getPort()) {
            promises[i]->set_value(0);
            continue;
        }
        confs->mPool->enqueue([=, &io_service, &promises](){
            try {
                cout << RED << path_to << RESET << endl;
                AsyncTcpClient client(io_service[i], ip, port, path, path_to, handleFinished, promises[i]);
                io_service[i].run();
            }
            catch (const exception &e) {
                cerr << "Exception in " << __PRETTY_FUNCTION__ << ": " << e.what() << "\n";
                // promises[i]->set_value(-1);
            }
        });
    }

    for (int i = 0; i < nodeNum; i++) {
        int res = futures[i].get();
        io_service[i].stop();
        if (res != 0) {
            string ip = get<0>(node.addresses[i]);
            string port = get<1>(node.addresses[i]);
            string dir = get<2>(node.addresses[i]);
            string path_to = dir + path.substr(confs->zk->localDir.size(), path.size());
            cout << "Failed to transfer file: " << ip << ":" << port << path_to << endl; 
        }
    }
    for (int i = 0; i < nodeNum; i++) {
        delete promises[i];
    }
}

int EventHandler::handleRemove(string path, Confs *confs, Event event)
{
    if (!fs::exists(path)) {
        cout << "path does not exists:" << path << '\n';
        return -1;
    }

    try {
        confs->notifier.ignoreFileOnce(fs::path(path));
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
            fs::rename(path_from, path_to);
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
    if (confs->isLocalEvent(ip, port)) {
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
        confs->pushLocalEvent(path_from);
        confs->pushLocalEvent(path_to);
        res = handleMove(path_from, path_to, confs);

    }
    else if (event.contains("event")) {
        string path = confs->convertPath(event["event"]["path"], mountPath);
        confs->pushLocalEvent(path);
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
