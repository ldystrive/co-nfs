#pragma once

#include <string>
#include <iostream>
#include <cstdlib>
#include <vector>

#include <boost/filesystem.hpp>

namespace mutils {

std::string exec(std::string cmd);
std::vector<std::string> split(std::string str, std::string delimiter);
bool isSubdir(boost::filesystem::path dir, boost::filesystem::path subdir);

}