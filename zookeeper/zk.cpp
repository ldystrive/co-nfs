#include <iostream>
#include <mutex>
#include <thread>
#include <future>


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