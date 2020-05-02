#include <iostream>
#include <cstdio>
#include <utility>
#include <vector>
#include <string>

#include <boost/filesystem.hpp>
#include <boost/optional.hpp>

#include "zookeeper/zk.h"
#include "zookeeper/zkCallback.h"
#include "co-nfs/confs.h"
#include "co-nfs/utils.h"
#include "inotify/inotifyBuilder.h"
#include "inotify/inotifyEvent.h"

using namespace std;
using namespace inotify;

static bool initLocalEnv(string localDir, boost::filesystem::path inFolder, boost::filesystem::path outFolder)
{
    // 判断localDir是否存在且是nfs的挂载点
    if (!boost::filesystem::exists(boost::filesystem::path(localDir))) {
        cerr << "localDir " << localDir << " not found." << endl;
        return false;
    }
    string str = mutils::exec(string("showmount -e"));
    vector<string> tokens = mutils::split(str, "\n");
    vector<string> dirs;
    for (int i = 1; i < tokens.size(); i++) {
        dirs.push_back(mutils::split(tokens[i], " ")[0]);
    }
    if (find(dirs.begin(), dirs.end(), localDir) == dirs.end()) {
        return false;
    }

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

Confs::Confs(const vector<string> &hosts, const string &localPath, const string &nodeName)
: notifier(BuildInotify())
{
    zk = ZkUtils::GetInstance();
    zhandle_t *zh = zk->init_handle(zk_init_cb, hosts);
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

    auto nodePtr = getNode(nodeName);
    if (!nodePtr) {
        cerr << "failed to pull node info." << endl;
        exit(0);
    }
    node = *nodePtr;
}

boost::optional<SharedNode> Confs::getNode(string nodeName)
{
    string path = zk->getNodePath();
    pair<int, vector<string>> addrs = zk->ls(path + "/addresses");
    if (addrs.first != 0) {
        return boost::none;
    }

    SharedNode node;
    node.nodeName = nodeName;
    
    // get addresses
    cout << "get addresses." << endl;
    for (auto str : addrs.second) {
        string ip = SharedNode::parseAddr(str);
        string dirPath;
        auto v = zk->get(path + "/addresses/" + str);
        if (v.first != 0) {
            return boost::none;
        }
        else {
            dirPath = v.second;
        }
        node.addresses.push_back({ip, dirPath});
    }

    // get ignore info
    cout << "get ignore info." << endl;
    auto v = zk->get(path + "/ignore");
    if (v.first != 0) {
        return boost::none;
    }
    else {
        node.ignore = v.second;
    }

    return node;
}

int Confs::watchLocalFiles()
{
    auto handleEvent = [&] (InotifyEvent inotifyEvent) {
        // 判断是否和临时文件夹有关
        if (mutils::isSubdir(inFolder, inotifyEvent.path) || 
            mutils::isSubdir(outFolder, inotifyEvent.path)) {
            return;
        }
        cout << "Event " << inotifyEvent.event << " on " << inotifyEvent.path << 
            " was triggered." << endl;
        string str = zk->localIp + "_" + inotifyEvent.toString();
        zk->create(zk->getNodePath() + "/events/event-", str, ZOO_EPHEMERAL_SEQUENTIAL);
    };

    auto handleMoveEvent = [&] (InotifyEvent inotifyEvent1, InotifyEvent inotifyEvent2) {
        // 判断是否和临时文件夹有关
        if (mutils::isSubdir(inFolder, inotifyEvent1.path)  || 
            mutils::isSubdir(outFolder, inotifyEvent1.path) ||
            mutils::isSubdir(inFolder, inotifyEvent2.path)  || 
            mutils::isSubdir(outFolder, inotifyEvent2.path)) {
            return;
        }
        string str = zk->localIp + "_" + inotifyEvent1.toString() + "_" + inotifyEvent2.toString();
        std::cout << "Event " << inotifyEvent1.event << " " << inotifyEvent1.path << " " << 
            inotifyEvent2.event << " " << inotifyEvent2.path << std::endl;
        zk->create(zk->getNodePath() + "/events/event-", str, ZOO_EPHEMERAL_SEQUENTIAL);
    };

    auto handleUnexpectedEvent = [&] (InotifyEvent inotifyEvent) {
        cout << "Event" << inotifyEvent.event << " on " << inotifyEvent.path <<
            " was triggered, but was not expected." << endl;
    };

    auto events = {
        Event::create,
        Event::create | Event::is_dir,
        Event::modify,
        Event::remove,
        Event::remove | Event::is_dir,
        Event::moved_from,
        Event::moved_to
    };
    cout << "watch path:" << zk->localDir << endl;
    notifier.onEvents(events, handleEvent)
            .onMoveEvent(handleMoveEvent)
            .onUnexpectedEvent(handleUnexpectedEvent)
            .watchpathRecursively(boost::filesystem::path(zk->localDir));
    inotifyThread = thread([&](){ notifier.run(); });
    return 0;
}