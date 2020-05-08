#include <iostream>
#include <cstdio>
#include <utility>
#include <vector>
#include <string>

#include <boost/filesystem.hpp>
#include <boost/optional.hpp>
#include <boost/lexical_cast.hpp>

#include "zookeeper/zk.h"
#include "zookeeper/zkCallback.h"
#include "co-nfs/confs.h"
#include "co-nfs/utils.h"
#include "co-nfs/handle.h"
#include "co-nfs/threadPool.h"
#include "inotify/inotifyBuilder.h"
#include "inotify/inotifyEvent.h"

#include "json/json.hpp"

using namespace std;
using namespace inotify;
using json = nlohmann::json;

static bool initLocalEnv(string localDir, boost::filesystem::path inFolder, boost::filesystem::path outFolder)
{
    // 判断localDir是否存在且是nfs的挂载点
    if (!boost::filesystem::exists(boost::filesystem::path(localDir))) {
        cerr << "localDir " << localDir << " not found." << endl;
        return false;
    }
#ifdef CHECK_NFS_MOUNT
    string str = mutils::exec(string("showmount -e"));
    vector<string> tokens = mutils::split(str, "\n");
    vector<string> dirs;
    for (int i = 1; i < tokens.size(); i++) {
        dirs.push_back(mutils::split(tokens[i], " ")[0]);
    }
    if (find(dirs.begin(), dirs.end(), localDir) == dirs.end()) {
        return false;
    }
#endif

    // 判断inFolder outFolder是否存在，并创建
    if (!boost::filesystem::is_directory(inFolder)) {
        boost::filesystem::create_directory(inFolder);
    }

    if (!boost::filesystem::is_directory(outFolder)) {
        boost::filesystem::create_directory(outFolder);
    }
    return true;
}

Confs::~Confs() 
{
    notifier.stop();
}

Confs::Confs(const vector<string> &hosts, const string &localPath, const string &nodeName, const string &port)
    : notifier(BuildInotify())
    , mPool(make_shared<ThreadPool>(THREADS_NUM))
    , port(port)
{
    zk = ZkUtils::GetInstance();
    zhandle_t *zh = zk->init_handle(zk_init_cb, hosts, port);
    assert(zh != NULL);

    zk->nodeName = nodeName;
    assert(zk->initLocalPath(localPath) == 0);

    inFolder = boost::filesystem::path(zk->localDir + "/.tmp_in");
    outFolder = boost::filesystem::path(zk->localDir + "/.tmp_out");

    if (!initLocalEnv(zk->localDir, inFolder, outFolder)) {
        string err = string("failed. ") + zk->localDir + " is not a nfs server point.";
        throw invalid_argument(string(err));
    }

    zk->createLayout();

    auto nodePtr = pullNode();
    if (!nodePtr) {
        throw invalid_argument("failed to pull node info.");
    }
    mNode = *nodePtr;
    
    auto handleFinished = [&](string str) {
        cout << str << "finished." << endl;
    };

    tcpServerThread = thread([&]() {
        fileTransfer = make_shared<AsyncTcpServer>(boost::lexical_cast<unsigned int>(port), handleFinished);
    });

}

string Confs::getPort()
{
    return port;
}

SharedNode Confs::getNode()
{
    boost::shared_lock<boost::shared_mutex> m(mNodeMutex);
    return mNode;
}

void Confs::setNode(SharedNode node)
{
    boost::unique_lock<boost::shared_mutex> m(mNodeMutex);
    this->mNode = node;
}

void Confs::updateAddresses()
{
    auto _addrs = pullAddresses();
    if (_addrs) {
        boost::unique_lock<boost::shared_mutex> m(mNodeMutex);
        this->mNode.addresses = move(*_addrs);
    }
}

void Confs::updateIgnore()
{
    auto _ignore = pullIgnore();
    if (_ignore) {
        auto files = mutils::split(*_ignore, ",");
        for (auto &file : files) {
            file = zk->localDir + "/" + file;
        }
        notifier.ignoreFiles(files);
        
        boost::unique_lock<boost::shared_mutex> m(mNodeMutex);
        this->mNode.ignore = move(*_ignore);
    }
}

void Confs::updateNode()
{
    auto _node = pullNode();
    if (_node) {
        setNode(*_node);
    }
}

void Confs::updateEventQueue()
{
    auto _eventQueue = pullEventQueue();
    if (_eventQueue) {
        eventQueue.setQueue(*_eventQueue);
    }
}

boost::optional<vector<tuple<string, string, string>>> Confs::pullAddresses()
{
    string path = zk->getNodePath() + "/addresses";
    auto value = zk->ls2(path);

    if (value.first != 0) {
        return boost::none;
    }
    
    vector<tuple<string, string, string>> res = {};
    for (auto &v : value.second) {
        auto addr = SharedNode::parseAddr(v.first);
        res.push_back(make_tuple(addr.first, addr.second, v.second));
    }
    
    return res;
}

boost::optional<vector<pair<string, string>>> Confs::pullEventQueue()
{
    string path = zk->getNodePath() + "/events";
    auto value = zk->ls2(path);
    if (value.first != 0) {
        return boost::none;
    }
    return value.second;
}

boost::optional<string> Confs::pullIgnore()
{
    string path = zk->getNodePath();
    cout << "get ignore info." << endl;
    auto v = zk->get(path + "/ignore");
    return v.first ? boost::none : boost::optional<string>(v.second);
}

boost::optional<SharedNode> Confs::pullNode()
{
    SharedNode node;
    node.nodeName = zk->nodeName;
    
    auto addrs = pullAddresses();
    auto ignore = pullIgnore();
    
    if (!addrs || !ignore) {
        return boost::none;
    }
    node.addresses = move(*addrs);
    node.ignore    = move(*ignore);
    return node;
}

bool Confs::isTriggeredByEventHandler(boost::filesystem::path path)
{
    auto value = eventQueue.getQueue();
    if (!value.empty()) {
        json j = json::parse(value[0].second);
        vector<boost::filesystem::path> eventPath;
        if (j.contains("event")) {
            eventPath.push_back(boost::filesystem::path(string(j["event"]["path"])));
        }
        else if (j.contains("event1") && j.contains("event2")) {
            eventPath.push_back(boost::filesystem::path(string(j["event1"]["path"])));
            eventPath.push_back(boost::filesystem::path(string(j["event2"]["path"])));
        }
        for (auto &p : eventPath) {
            string str1 = path.string();
            string str2 = p.string();
            if (str1.find(str2) != str1.npos) {
                return true;
            }
        }
    }
    return false;
}

int Confs::watchLocalFiles()
{
    auto handleEvent = [&] (InotifyEvent inotifyEvent) {
        // 判断是否和临时文件夹有关
        if (mutils::isSubdir(inFolder, inotifyEvent.path) || 
            mutils::isSubdir(outFolder, inotifyEvent.path)) {
            return;
        }
        if (isTriggeredByEventHandler(inotifyEvent.path)) {
            return;
        }
        std::cout << "Event " << inotifyEvent.event << " on " << inotifyEvent.path << 
            " was triggered." << endl;
        json j = {
            {"ip", zk->localIp},
            {"port", port},
            {"path", zk->localDir},
            {"event", inotifyEvent.toJson()}
        };
        zk->create(zk->getNodePath() + "/events/event-", j.dump(), ZOO_EPHEMERAL_SEQUENTIAL);
    };

    auto handleMoveEvent = [&] (InotifyEvent inotifyEvent1, InotifyEvent inotifyEvent2) {
        // 判断是否和临时文件夹有关
        if (mutils::isSubdir(inFolder, inotifyEvent1.path)  || 
            mutils::isSubdir(outFolder, inotifyEvent1.path) ||
            mutils::isSubdir(inFolder, inotifyEvent2.path)  || 
            mutils::isSubdir(outFolder, inotifyEvent2.path)) {
            return;
        }
        if (isTriggeredByEventHandler(inotifyEvent1.path)
            || isTriggeredByEventHandler(inotifyEvent2.path)) {
            return;
        }
        json j = {
            {"ip", zk->localIp},
            {"port", port},
            {"path", zk->localDir},
            {"event1", inotifyEvent1.toJson()},
            {"event2", inotifyEvent2.toJson()}
        };
        std::cout << "Event " << inotifyEvent1.event << " " << inotifyEvent1.path << " " << 
            inotifyEvent2.event << " " << inotifyEvent2.path << std::endl;
        zk->create(zk->getNodePath() + "/events/event-", j.dump(), ZOO_EPHEMERAL_SEQUENTIAL);
    };

    auto handleOpenEvent = [&] (InotifyEvent inotifyEvent) {
        if (!boost::filesystem::is_regular_file(inotifyEvent.path)) {
            return;
        }
        std::cout << "open Event." << " on " << inotifyEvent.path << std::endl;
    };

    auto handleCloseNoWriteEvent = [&] (InotifyEvent inotifyEvent) {
        if (!boost::filesystem::is_regular_file(inotifyEvent.path)) {
            return;
        }
        std::cout << "closeNoWrite Event." << " on " << inotifyEvent.path << std::endl;
    };

    auto handleCloseWriteEvent = [&] (InotifyEvent inotifyEvent) {
        if (!boost::filesystem::is_regular_file(inotifyEvent.path)) {
            return;
        }
        std::cout << "closeWrite Event." << " on " << inotifyEvent.path << std::endl;
    };

    auto handleUnexpectedEvent = [&] (InotifyEvent inotifyEvent) {
        cout << "Event" << inotifyEvent.event << " on " << inotifyEvent.path <<
            " was triggered, but was not expected." << endl;
    };

    auto events = {
        Event::create,
        Event::create | Event::is_dir,
        Event::close_write,
        Event::remove,
        Event::remove | Event::is_dir,
        Event::moved_from,
        Event::moved_to
    };

    auto openEvents = {
        Event::open,
        Event::open | Event::is_dir
    };

    auto closeNoWriteEvents = {
        Event::close_nowrite,
        Event::close_nowrite | Event::is_dir
    };

    auto closeWriteEvents = {
        Event::close_write,
        Event::close_write | Event::is_dir
    };

    std::cout << "watch path:" << zk->localDir << endl;
    notifier.onEvents(events, handleEvent)
            // .onEvents(openEvents, handleOpenEvent)
            // .onEvents(closeNoWriteEvents, handleCloseNoWriteEvent)
            // .onEvents(closeWriteEvents, handleCloseWriteEvent)
            .onMoveEvent(handleMoveEvent)
            .onUnexpectedEvent(handleUnexpectedEvent)
            .watchpathRecursively(boost::filesystem::path(zk->localDir));
    inotifyThread = thread([&](){ notifier.run(); });
    return 0;
}

int Confs::watchServerInfo()
{
    string path = zk->getNodePath();
    // addresses
    string addrPath = path + "/addresses";
    zoo_awget_children(zk->zh, addrPath.c_str(), addresses_cb, this, future_strings_completion_cb, NULL);

    // events
    string eventPath = path + "/events";
    zoo_awget_children(zk->zh, eventPath.c_str(), events_cb, this, future_strings_completion_cb, NULL);

    // ignore
    string ignorePath = path + "/ignore";
    zoo_awexists(zk->zh, ignorePath.c_str(), ignore_cb, this, future_rc_completion_cb, NULL);

    return 0;
}

void Confs::consistencyCheck()
{
    auto events = eventQueue.getQueue();

    auto handler = [&](const pair<string, string> &event, int type = -1) {

        json e = json::parse(event.second);
        string eventId = event.first;
        string path = e["event"]["path"];
        Event v = static_cast<Event>(static_cast<uint32_t>(stoul(string(e["event"]["event"]))));
        if (type == -1) {
            if ((v & Event::create) != Event::none || (v & Event::close_write) != Event::none) {
                type = 1;
            }
            else {
                type = 0;
            }
        }

        // type = {0, 1}, remove event
        string zkEventPath = zk->getNodePath() + "/events/" + eventId;
        zk->removeRecursively(zkEventPath);

        // type = 1, remove event and back up
        if (type == 1) {
            boost::filesystem::copy_file(path, path + "_copy",
                boost::filesystem::copy_option::overwrite_if_exists);
        }
    };

    auto checker = [&](const pair<string, string> &event) -> bool {

        json e2 = json::parse(event.second);
        string ip2 = e2["ip"];
        string dirPath2 = e2["path"];
        string p2 = e2["event"]["path"];
        Event v2 = static_cast<Event>(static_cast<uint32_t>(stoul(string(e2["event"]["event"]))));
        
        // not a directory
        if (boost::filesystem::is_directory(boost::filesystem::path(p2))) {
            return false;
        }

        // close_write or create
        if ((v2 & Event::close_write) == Event::none 
            || (v2 & Event::create) == Event::none
            || (v2 & Event::remove) == Event::none) {
            return false;
        }

        for (const auto &preEvent: events) {
            if (preEvent.first == event.first) {
                continue;
            }
            json e1 = json::parse(preEvent.second);
            if (e1["ip"] == ip2 && e1["path"] == dirPath2) {
                continue;
            }
            if (!e1.contains("event")) {
                continue;
            }
            string p1 = e1["event"]["path"];
            Event v1 = static_cast<Event>(static_cast<uint32_t>(e1["event"]["event"]));
            if (p1 != p2) {
                continue;
            }
            return true;
        }
        return false;
    };

    if (!events.empty()) {
        auto lastIter = --events.end();
        string eventId = (*lastIter).first;
        json event = json::parse((*lastIter).second);
        if (event["ip"] != zk->localIp || event["path"] != zk->localDir) {
            return;
        }
        if (event.contains("event")) {
            if (checker(event)) {
                handler(event);
            }
        }
        else if (event.contains("event1") && event.contains("event2")) {
            // not do anything.
        }
        else {
            // unknown event.
        }
    }
}

string Confs::convertPath(string rawPath, string baseDir)
{
    return this->zk->localDir + rawPath.substr(baseDir.length(), rawPath.size());
}
