#include <iostream>
#include <utility>
#include <future>

#include "zookeeper/zk.h"

using namespace std;

void zk_init_cb(zhandle_t* zh, int type, int state, const char* path, void *watcherCtx)
{
    cout << __PRETTY_FUNCTION__ << endl;
    cout << "zh:" << zh << endl;
    cout << "type:" << type << endl;
    cout << "state:" << state << endl;
    cout << "path:" << path << endl;
    promise<int> *prom = static_cast<promise<int> *>(watcherCtx);
    prom->set_value(state);
}

void future_rc_completion_cb(int rc, const struct Stat *stat, const void *data)
{
    cout << __PRETTY_FUNCTION__ << endl;
    cout << "error code:" << rc << endl;
    if (data == NULL) {
        return;
    }
    promise<int> *prom = static_cast<promise<int> *>(const_cast<void *>(data));
    prom->set_value(rc);
}

void future_string_completion_cb(int rc, const char *value, const void *data)
{
    cout << __PRETTY_FUNCTION__ << endl;
    cout << "error code:" << rc << endl;
    cout << "value:" << value << endl;
    if (data == NULL) {
        return;
    }
    promise<pair<int, string> > *prom = static_cast<promise<pair<int, string> > *>(const_cast<void *>(data));
    prom->set_value(pair<int, string>(rc, string(value)));
}