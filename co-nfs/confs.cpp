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

using namespace std;

Confs::~Confs() 
{
    inotifyBuilder.stop();
}

Confs::Confs(const vector<string> &hosts, const string &localPath, const string &nodeName)
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