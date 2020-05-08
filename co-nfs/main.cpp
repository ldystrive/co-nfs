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
#include "co-nfs/confs.h"

using namespace std;
namespace bpo = boost::program_options;

// #define DEBUG

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
    string localPath;
    vector<string> zoo_hosts;
    string nodeName;
    string port;

    opt.add_options()
        ("localIp,l", bpo::value<string>(&localPath)->default_value("192.168.137.132/shareData", "local path"))
        ("server,s", bpo::value<vector<string> >()->multitoken(), "zoo server addresses")
        ("port,p", bpo::value<string>(&port)->default_value("5566"), "local port")
        ("name,n", bpo::value<string>(&nodeName)->default_value("node1"), "node name");
    
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

    // init(zoo_hosts, localPath, nodeName);
    try {
        Confs confs(zoo_hosts, localPath, nodeName, port);
        confs.watchLocalFiles();
        confs.watchServerInfo();
#ifdef DEBUG
        using json = nlohmann::json;
        json event1 = {
            {"path", "/shareData/a.txt"},
            {"event", "0"}
        };
        json event2 = {
            {"path", "/shareData/b.txt"},
            {"event", "0"}
        };
        json event = {
            {"ip", "2333"},
            {"path", "/shareData"},
            {"event1", event1},
            {"event2", event2}
        };
        json eventS = {
            {"path", "/shareData/test_create_dir"},
            {"event", to_string(static_cast<uint32_t>(inotify::Event::remove | inotify::Event::is_dir))}
        };
        json eventS1 = {
            {"ip", "2333"},
            {"path", "/shareData"},
            {"event", eventS},
        };
        confs.eventHandler.solveEvent("test_for_mv_file", eventS1, &confs);
#endif
        confs.inotifyThread.join();
        confs.tcpServerThread.join();
    }
    catch(const exception &e) {
        cerr << e.what() << endl;
        exit(0);
    }
    
    while(true) {
        this_thread::sleep_for(chrono::minutes(5));
    }

    return 0;
}
