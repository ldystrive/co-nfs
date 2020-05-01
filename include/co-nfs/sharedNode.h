#pragma once

#include <iostream>
#include <cstdlib>
#include <cassert>
#include <cstdio>
#include <vector>
#include <string>
#include <utility>

using namespace std;

class SharedNode {
public:
    ~SharedNode();
    SharedNode();
    SharedNode(const string &name, const string &ignore, const vector<pair<string, string>> &addrs);
    
    // str likes: 192.168.137.132_1567464848020645536, ip_hash(mount path)
    static string parseAddr(const string &str);

public:
    string nodeName;
    string ignore;
    // pair<string, string>: ip & mount path
    vector<pair<string, string> > addresses;
    
};