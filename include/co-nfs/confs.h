#pragma once

#include <iostream>
#include <vector>
#include <string>

#include <boost/optional.hpp>

#include "sharedNode.h"
#include "../zookeeper/zk.h"
#include "../inotify/inotifyBuilder.h"

class Confs {
public:
    ~Confs();
    Confs(const std::vector<std::string> &hosts,
        const std::string &localPath,
        const std::string &nodeName);
    
    boost::optional<SharedNode> getNode(std::string nodeName);

public:
    SharedNode node;

private:
    Confs();
    ZkUtils *zk;
    inotify::InotifyBuilder inotifyBuilder;
};