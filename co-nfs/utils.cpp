#include <string>
#include <iostream>
#include <cstdlib>
#include <vector>

#include "co-nfs/utils.h"

using namespace std;

namespace mutils {

string exec(string cmd)
{
    FILE * pipe = popen(cmd.c_str(), "r");
    if (!pipe) return {};
    char buffer[128];
    string res = {};
    while (!feof(pipe)) {
        if (fgets(buffer, 128, pipe) != NULL) {
            res += buffer;
        }
    }
    pclose(pipe);
    return res;
}

vector<string> split(string str, string delimiter)
{
    size_t pos = 0;
    vector<string> tokens;
    while ((pos = str.find(delimiter)) != string::npos) {
        tokens.push_back(str.substr(0, pos));
        str.erase(0, pos + delimiter.length());
    }
    if (!str.empty()) {
        tokens.push_back(str);
    }
    return tokens;
}

}