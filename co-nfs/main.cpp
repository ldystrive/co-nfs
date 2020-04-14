#include <iostream>
#include <string>
#include <vector>
#include <cassert>
#include <csignal>
#include <thread>
#include <chrono>

#include <boost/program_options.hpp>

#include "zookeeper/zk.h"
#include "zookeeper/zkCallback.h"
#include "inotify/inotifyUtils.h"
#include "co-nfs/handle.h"

using namespace std;
namespace bpo = boost::program_options;

void signalHandler(int signum)
{
    cout << "Interrupt signal (" << signum << ") received.\n" << endl;

    ZkUtils *zk = ZkUtils::GetInstance();
    int res = zookeeper_close(zk->zh);
    cout << "close zookeeper:" << zerror(res) << endl;
    zk->deleteInstance();
    exit(signum);
}

int main(int argc, char *argv[])
{
    bpo::options_description opt("all options");
    string localIp;
    vector<string> zoo_hosts;

    opt.add_options()
        ("localIp,l", bpo::value<string>(&localIp)->default_value("127.0.0.1", "local ip"))
        ("server,s", bpo::value<vector<string> >()->multitoken(), "zoo server addresses");
    
    bpo::variables_map vm;
    try {
        bpo::store(parse_command_line(argc, argv, opt), vm);
    }
    catch(...) {
        cout << "error: cannot parse args!" << endl;
        return 0;
    }
    bpo::notify(vm);
    if (vm.count("server")) {
        for (auto host : vm["server"].as<vector<string> >()) {
            zoo_hosts.push_back(host);
        }
    }
    else {
        zoo_hosts.push_back("127.0.0.1:2181");
    }

    signal(SIGINT, signalHandler);

    init(zoo_hosts, localIp);
    
    while(true) {
        this_thread::sleep_for(chrono::minutes(5));
    }

    return 0;
}