#include <iostream>
#include <mutex>
#include <thread>
#include <future>
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

zhandle_t *ZkUtils::init_handle(watcher_fn fn, const vector<string> &hosts)
{
    string mhost;
    for (const auto &host : hosts) {
        m_hosts.push_back(host);
        if (mhost == "")
            mhost = host;
        else
            mhost += string(",") + host;
    }

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

int ZkUtils::exists(string path)
{
    promise<int> *prom = new promise<int>();
    future<int> future = prom->get_future();
    int res = zoo_aexists(this->zh, path.c_str(), 0, future_rc_completion_cb, (void *)prom);
    int rc = future.get();
    delete prom;
    if (res == ZOK) {
        return rc;
    }
    return res;
}

int ZkUtils::create(string path, string value)
{
    promise<pair<int, string> > *prom = new promise<pair<int, string> >();
    future<pair<int, string> > future = prom->get_future();
    int res = zoo_acreate(this->zh, path.c_str(), value.c_str(), value.length(), \
        &ZOO_OPEN_ACL_UNSAFE, ZOO_PERSISTENT, future_string_completion_cb, (void *)prom);
    pair<int, string> v = future.get();
    delete prom;
    if (res == ZOK) {
        return v.first;
    }
    return res;
}

int ZkUtils::checkAndCreate(string path, string value)
{
    int res = this->exists(path);
    
    if (res == ZNONODE) {
        res = this->create(path, value);
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
#endif

}

void ZkUtils::createSharedNode(string nodeName, const vector<pair<string, string> > &addresses, const vector<string> &ignore)
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
        this->checkAndCreate(prefix + "/addresses/" + ip + "_" + to_string(stringHash(mount)), mount);
    }
    this->checkAndCreate(prefix + "/events", "");

}