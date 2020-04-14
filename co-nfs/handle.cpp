#include "zookeeper/zk.h"

void zk_init_cb(zhandle_t* zh, int type, int state, const char* path, void *watcherCtx)
{
    cout << __PRETTY_FUNCTION__ << endl;
    cout << "zh:" << zh << endl;
    cout << "type:" << type << endl;
    cout << "state:" << state << endl;
    cout << "path:" << path << endl;
    string *ctx = static_cast<string*>(watcherCtx);
    cout << "watcherCtx:" << *ctx << endl;
    delete ctx;
}