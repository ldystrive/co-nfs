#include <iostream>

#include <string>
#include <vector>
#include <csignal>
#include <thread>
#include <chrono>

#include "zookeeper/zk.h"
#include "inotify/inotifyUtils.h"
#include "co-nfs/handle.h"

using namespace std;

void signalHandler(int signum)
{
    cout << "Interrupt signal (" << signum << ") received.\n" << endl;

    exit(signum);
}

void init(const vector<string> &hosts) {
    
    ZkUtils *zk = ZkUtils::GetInstance();
    zk->init_handle(zk_init_cb, hosts);

}

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

    signal(SIGINT, signalHandler);

    init(zoo_hosts);
    
    while(true) {
        this_thread::sleep_for(chrono::minutes(5));
    }

    return 0;
}