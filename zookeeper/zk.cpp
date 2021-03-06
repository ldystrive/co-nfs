#include <iostream>
#include <mutex>
#include <thread>
#include <future>
#include <stack>
#include <queue>
#include <functional>
#include <utility>

#include "zookeeper/zkCallback.h"
#include "zookeeper/zk.h"

using namespace std;

ZkUtils *ZkUtils::m_sInstance = NULL;
mutex ZkUtils::m_mux;

const int ZkUtils::timeout = 15000;

ZkUtils *&ZkUtils::GetInstance()
{
    if (m_sInstance == NULL) {
        unique_lock<mutex> lock(m_mux);
        if (m_sInstance == NULL) {
            m_sInstance = new (std::nothrow) ZkUtils();
        }
    }
    return m_sInstance;
}

void ZkUtils::deleteInstance()
{
    unique_lock<mutex> lock(m_mux);
    if (m_sInstance) {
        delete m_sInstance;
        m_sInstance = NULL;
    }
}

ZkUtils::ZkUtils() {}
ZkUtils::~ZkUtils() {}

string ZkUtils::getNodePath()
{
    return string("/co_nfs/shared_nodes/") + nodeName;
}

zhandle_t *ZkUtils::init_handle(watcher_fn fn, const vector<string> &hosts, string port)
{
    string mhost;
    for (const auto &host : hosts) {
        m_hosts.push_back(host);
        if (mhost == "")
            mhost = host;
        else
            mhost += string(",") + host;
    }
    this->port = port;

    promise<int> *prom = new promise<int>();
    future<int> future = prom->get_future();
    this->zh = zookeeper_init(mhost.c_str(), fn, this->timeout, 0, prom, 0);
    int state = future.get();
    delete prom;
    if (state != ZOO_CONNECTED_STATE) {
        cout << "error: failed to server:" << mhost << endl;
        return NULL;
    }
    return this->zh;
}

int ZkUtils::initLocalPath(const string &str)
{
    int pos = str.find('/');
    if (pos == str.npos) {
        return -1;
    }
    localIp = str.substr(0, pos);
    localDir = str.substr(pos, str.size());
    while (localDir.length() > 1 && localDir[localDir.length() - 1] == '/') {
        localDir = localDir.substr(0, localDir.length() - 1);
    }
    cout << "localPath:" << localIp << localDir << endl;
    return 0;
}

int ZkUtils::exists(string path)
{
    promise<int> *prom = new promise<int>();
    future<int> future = prom->get_future();
    int res = zoo_aexists(this->zh, path.c_str(), 0, future_rc_completion_cb, (void *)prom);
    int rc = future.get();
    if (res == ZOK) {
        delete prom;
        return rc;
    }
    delete prom;
    return res;
}

int ZkUtils::create(string path, string value, const int mode)
{
    promise<pair<int, string> > *prom = new promise<pair<int, string> >();
    future<pair<int, string> > future = prom->get_future();
    int res = zoo_acreate(this->zh, path.c_str(), value.c_str(), value.length(), \
        &ZOO_OPEN_ACL_UNSAFE, mode, future_string_completion_cb, (void *)prom);
    pair<int, string> v = future.get();
    if (res == ZOK) {
        delete prom;
        return v.first;
    }
    delete prom;
    return res;
}

int ZkUtils::checkAndCreate(string path, string value, const int mode)
{
    int res = this->exists(path);
    
    if (res == ZNONODE) {
        res = this->create(path, value, mode);
    }
    return res;
}

void ZkUtils::createLayout()
{
    // int res = this->checkAndCreate(string("/test2333"), string("test for create node"));
    int res = 0;
    this->checkAndCreate(string("/co_nfs"), string(""));
    this->checkAndCreate(string("/co_nfs/shared_nodes"), string(""));
    this->checkAndCreate(string("/co_nfs/node_ip_map"), string(""));
// 添加数据用于开发
#ifdef _DEV
    vector<pair<string, string> > addresses = {{string("192.168.137.132"), string("/data")},
                                               {string("192.168.137.133"), string("/data")}};
    vector<string> ignore = {"test_ignore1.txt", "test_ignore2.txt"};
    this->createSharedNode(string("node1"), addresses, ignore);
#else
    this->createSharedNode(this->nodeName, { {this->localIp, this->localDir} },
        {".tmp_in", ".tmp_out"});
#endif

}

void ZkUtils::createSharedNode(string nodeName, const vector<pair<string, string> > &addresses,
    const vector<string> &ignore)
{
    // this->checkAndCreate(string("/co_nfs/node_ip_map/") + )
    string prefix = string("/co_nfs/shared_nodes/") + nodeName;
    this->checkAndCreate(prefix, string(""));
    
    string ignoreFiles = string();
    for (const auto &i : ignore) {
        if (ignoreFiles == "") ignoreFiles = i;
        else ignoreFiles += string(",") + i;
    }
    this->checkAndCreate(prefix + "/ignore", ignoreFiles);

    this->checkAndCreate(prefix + "/addresses", string());
    hash<string> stringHash;
    for (const auto &address : addresses) {
        string ip = address.first;
        string mount = address.second;
        this->checkAndCreate(prefix + "/addresses/" + ip + "_" + port,
            mount, ZOO_EPHEMERAL);
    }
    this->checkAndCreate(prefix + "/events", "");
    // this->removeRecursively(prefix + "/eventState");
    this->checkAndCreate(prefix + "/eventState", "");
}

pair<int, vector<string> > ZkUtils::ls(const string &path)
{
    promise<pair<int, vector<string>>> *prom = new promise<pair<int, vector<string>>>();
    future<pair<int, vector<string>>> future = prom->get_future();
    int res = zoo_aget_children(this->zh, path.c_str(), 0, future_strings_completion_cb, prom);
    pair<int, vector<string>> v = future.get();
    if (res == ZOK) {
        delete prom;
        return v;
    }
    delete prom;
    return {res, {}};
}

pair<int, vector<pair<string, string>>> ZkUtils::ls2(const string &path)
{
    auto addrs = ls(path);
    if (addrs.first != 0) {
        return {addrs.first, {}};
    }
    vector<pair<string, string>> res;
    for (auto nodeName : addrs.second) {
        auto value = get(path + "/" + nodeName);
        if (value.first != 0) {
            return {value.first, {}};
        }
        res.push_back({nodeName, value.second});
    }
    return {0, res};
}

pair<int, string> ZkUtils::get(const string &path)
{
    promise<pair<int, vector<uint8_t>>> *prom = new promise<pair<int, vector<uint8_t>>>();
    future<pair<int, vector<uint8_t>>> future = prom->get_future();
    int res = zoo_aget(this->zh, path.c_str(), 0, future_data_completion_cb, prom);
    pair<int, vector<uint8_t>> v = future.get();
    if (res ==ZOK) {
        delete prom;
        return {v.first, string(v.second.begin(), v.second.end())};
    }
    delete prom;
    return {res, {}};
}

void ZkUtils::setNodeWatcher(const string &path, void *ctx)
{
    zoo_awexists(this->zh, path.c_str(), set_watcher_cb, ctx, future_rc_completion_cb, NULL);
}

void ZkUtils::setChildrenWatcher(const string &path, void *ctx)
{
    zoo_awget_children(this->zh, path.c_str(), set_watcher_cb, ctx, future_strings_completion_cb, NULL);
}

int ZkUtils::remove(string path)
{
    promise<int> *prom = new promise<int>();
    future<int> future = prom->get_future();
    int res = zoo_adelete(this->zh, path.c_str(), -1, future_void_completion_cb, prom);
    int v = future.get();
    if (res == ZOK) {
        delete prom;
        return v;
    }
    delete prom;
    return res;
}

int ZkUtils::removeRecursively(string path)
{
    cout << "recursively remove path: " << path << endl;
    queue<string> pathQueue;
    stack<string> pathStack;
    pathQueue.push(path);
    while (!pathQueue.empty()) {
        string path1 = pathQueue.front();
        cout << "found path: " << path1 << endl; 
        pathQueue.pop();
        pathStack.push(path1);
        auto res = ls(path1);
        if (res.first != 0) {
            return res.first;
        }
        for (auto subDir : res.second) {
            pathQueue.push(path1 + "/" + subDir);
        }
    }
    cout << "start remove" << endl;
    while (!pathStack.empty()) {
        string path1 = pathStack.top();
        pathStack.pop();
        int res = remove(path1);
        if (res != 0) {
            continue;
        }
    }
}
