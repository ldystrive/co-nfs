#include <iostream>
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