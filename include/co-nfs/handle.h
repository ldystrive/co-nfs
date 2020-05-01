#pragma once

#include <string>
#include <vector>

#include "../zookeeper/zk.h"

void init(const std::vector<std::string> &hosts, const std::string &localPath, const std::string &nodeName);