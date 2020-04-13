#include <zookeeper/zookeeper.h>
#include <iostream>
#include <cstdlib>
#include <cassert>
#include <cstdio>
#include <thread>
#include <condition_variable>
#include <mutex>
#include <chrono>

using namespace std;

#define host "172.18.0.2:2181"
const string root_path("/");

volatile bool connected = false;

condition_variable cond;
mutex mux;

void signal_on_request_finished()
{
    unique_lock<mutex> lock(mux);
    connected = true;
    cond.notify_all();
}

void wait_until_connected()
{
    unique_lock<mutex> lock(mux);
    while (!connected) {
        cond.wait(lock);
    }
}

void zk_event_cb(zhandle_t* zh, int type, int state, const char* path, void *watcherCtx)
{
    cout << __PRETTY_FUNCTION__ << endl;
    cout << "zh:" << zh << endl;
    cout << "type:" << type << endl;
    cout << "state:" << state << endl;
    cout << "path:" << path << endl;
    string *ctx = static_cast<string*>(watcherCtx);
    cout << "watcherCtx:" << *ctx << endl;
    delete ctx;
    signal_on_request_finished();
}

zhandle_t *init_handle()
{
    cout << __PRETTY_FUNCTION__ << endl;
    string *str = new string(__PRETTY_FUNCTION__);
    return zookeeper_init(
        (host + root_path).c_str(),
        zk_event_cb,
        15000,
        NULL,
        str,
        0);
}

void close_zhandle(zhandle_t *&handle)
{
    cout << __PRETTY_FUNCTION__ << endl;
    int res = zookeeper_close(handle);
    cout << res << "\t" << zerror(res) << endl;
    handle = NULL;
}

void create_completion_cb(int rc, const char *value, const void *data)
{
    cout << __PRETTY_FUNCTION__ << endl;
    cout << "rc:" << rc << endl;
    cout << "value:" << value << endl;
}

void create(zhandle_t* zh, const char * path) 
{
    cout << __PRETTY_FUNCTION__ << endl;
    string data("data_by_API_zoo_create");
    char buffer[64] = {};
    int res = zoo_acreate(zh, path, data.c_str(), data.length(), &ZOO_OPEN_ACL_UNSAFE, \
        ZOO_EPHEMERAL, create_completion_cb, NULL);
    cout << "res:" << res << endl;
}

void set_completion_cb(int rc, const struct Stat *stat, const void *data)
{
    cout << __PRETTY_FUNCTION__ << endl;
    cout << "stat:" << stat->dataLength  << endl;
}

void set(zhandle_t* zh, const char *path, const char *buffer)
{
    cout << __PRETTY_FUNCTION__ << endl;

    int res = zoo_aset(zh, path, buffer, sizeof(buffer), -1, set_completion_cb, NULL);
    cout << "res:" << res << endl;
}

int main(int argc, char *argv[])
{
    zhandle_t *handle = init_handle();
    assert(handle != NULL);
    wait_until_connected();
    create(handle, "/test1");
    string data("23333");
    set(handle, "/test1", data.c_str());
    this_thread::sleep_for(chrono::seconds(10));
    close_zhandle(handle);
    return 0;
}