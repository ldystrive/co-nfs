#include <iostream>
#include <cstdio>
#include <utility>
#include <vector>
#include <string>

#include <boost/filesystem.hpp>

#include "zookeeper/zk.h"
#include "zookeeper/zkCallback.h"
#include "co-nfs/confs.h"
#include "co-nfs/handle.h"
#include "co-nfs/utils.h"
#include "co-nfs/sharedNode.h"

using namespace std;

void addresses_cb(zhandle_t *zh, int type, int state, const char *path, void *ctx)
{
    cout << __PRETTY_FUNCTION__ << endl;
    cout << "type:" << type << " state:" << state << " path:" << path << endl;
    zoo_awget_children(zh, path, addresses_cb, ctx, future_strings_completion_cb, NULL);
    
    Confs *confs = static_cast<Confs *>(ctx);

    confs->mPool->enqueue([confs](){
        confs->updateAddresses();
        SharedNode node = confs->getNode();
        cout << "new addresses:" << endl;
        for (auto a : node.addresses) {
            cout << a.first << ' ' << a.second << endl;
        }
    });
}

void events_cb(zhandle_t *zh, int type, int state, const char *path, void *ctx)
{
    cout << __PRETTY_FUNCTION__ << endl;
    cout << "type:" << type << " state:" << state << " path:" << path << endl;
    zoo_awget_children(zh, path, events_cb, ctx, future_strings_completion_cb, NULL);

    Confs *confs = static_cast<Confs *>(ctx);

    confs->mPool->enqueue([confs](){
        confs->updateEventQueue();
        auto value = confs->eventQueue.getQueue();
        cout << "new event queue:" << endl;
        for (auto a : value) {
            cout << a.first << ' ' << a.second << endl;
        }
    });

}

void ignore_cb(zhandle_t *zh, int type, int state, const char *path, void *ctx)
{
    cout << __PRETTY_FUNCTION__ << endl;
    cout << "type:" << type << " state:" << state << " path:" << path << endl;

    zoo_awexists(zh, path, ignore_cb, ctx, future_rc_completion_cb, NULL);

    Confs *confs = static_cast<Confs *>(ctx);
    confs->mPool->enqueue([confs](){
        confs->updateIgnore();
        SharedNode node = confs->getNode();
        cout << "new ignore:" << node.ignore << endl;
    });
}