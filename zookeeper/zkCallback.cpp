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
    if (rc != 0) {
        prom->set_value({rc, ""});
        return;
    }
    prom->set_value(pair<int, string>(rc, string(value)));
}

void future_strings_completion_cb(int rc, const struct String_vector *strings, const void *data)
{
    cout << __PRETTY_FUNCTION__ << endl;
    cout << "error code:" << rc << endl;
    if (data == NULL) {
        return;
    }
    promise<pair<int, vector<string>>> *prom = \
        static_cast<promise<pair<int, vector<string>>> *>(const_cast<void *>(data));
    if (rc != 0) {
        prom->set_value({rc, {}});
        return;
    }
    vector<string> strs;
    for (int i = 0; i < strings->count; i++) {
        strs.push_back(strings->data[i]);
    }
    prom->set_value(pair<int, vector<string>>(rc, strs));
}

void future_data_completion_cb(int rc, const char *value, int value_len, const struct Stat *stat, const void *data)
{
    cout << __PRETTY_FUNCTION__ << endl;
    cout << "error code:" << rc << endl;
    if (data == NULL) {
        return;
    }
    promise<pair<int, vector<uint8_t>>> *prom = \
        static_cast<promise<pair<int, vector<uint8_t>>> *>(const_cast<void *>(data));
    if (rc != 0) {
        prom->set_value({rc, {}});
        return;
    }
    vector<uint8_t> arr(value, value+value_len);
    prom->set_value(pair<int, vector<uint8_t>>(rc, arr));

}


void set_watcher_cb(zhandle_t *zh, int type, int state, const char *path, void *ctx)
{
    cout << __PRETTY_FUNCTION__ << endl;
    cout << "type:" << type << endl;
    cout << "state:" << state << endl;
    cout << "path:" << path << endl;

    // TODO: parse ctx and do sth.
    // register a new watcher
    if (type == 3) {
        zoo_awexists(zh, path, set_watcher_cb, ctx, future_rc_completion_cb, NULL);
    }
    else if (type == 4) {
        zoo_awget_children(zh, path, set_watcher_cb, ctx, future_strings_completion_cb, NULL);
    }
}
