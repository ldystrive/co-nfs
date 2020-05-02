#pragma once

#include <string>
#include <iostream>
#include <cstdlib>
#include <vector>

namespace mutils {

std::string exec(std::string cmd);
std::vector<std::string> split(std::string str, std::string delimiter);

}