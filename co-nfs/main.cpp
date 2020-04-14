#include <iostream>

#include <string>
#include <vector>

#include "zookeeper/zk.h"
#include "inotify/inotifyUtils.h"

using namespace std;

int main(int argc, char *argv[])
{
    vector<string> zoo_hosts;
    if (argc == 1) {
        zoo_hosts.push_back("127.0.0.1:2181");
    }
    else {
        for (int i = 1; i < argc; i++) {
            zoo_hosts.push_back(string(argv[i]));
        }
    }

    

    return 0;
}