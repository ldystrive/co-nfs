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
#include "inotify/inotifyBuilder.h"
#include "inotify/inotifyEvent.h"

using namespace std;
using namespace inotify;

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
        cout << "Event " << inotifyEvent.event << " on " << inotifyEvent.path << 
            " was triggered." << endl;
    };

    auto handleMoveEvent = [&] (InotifyEvent inotifyEvent1, InotifyEvent inotifyEvent2) {
        std::cout << "Event " << inotifyEvent1.event << " " << inotifyEvent1.path << " " << 
            inotifyEvent2.event << " " << inotifyEvent2.path << std::endl;
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