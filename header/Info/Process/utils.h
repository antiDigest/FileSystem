
/*
    @author: antriksh
    Version 1: 4/11/2018
*/

#include "../../includes.h"
#include "ProcessInfo.h"

// Read and store information of all processes in a given fileName
vector<ProcessInfo> readClients(vector<ProcessInfo> clients, string fileName) {
    ifstream clientFile(fileName);
    string line;
    while (getline(clientFile, line)) {
        if (line.c_str()[0] == '#') continue;
        // ProcessInfo client;
        ProcessInfo c;
        stringstream ss(line);
        string item;
        getline(ss, item, ',');
        c.processID = item;
        getline(ss, item, ',');
        c.hostname = item;
        getline(ss, item, ',');
        c.port = stoi(item);
        getline(ss, item, ',');
        c.system = item;
        // client.store(c);
        clients.push_back(c);
    }
    return clients;
}

// Find an ID in a vector of clients/servers
ProcessInfo findInVector(vector<ProcessInfo> clients, string name) {
    for (ProcessInfo client : clients) {
        if (client.processID == name) return client;
    }
    throw "Process Not found";
}

// Find the index of ID in a vector of clients/servers
int findServerIndex(vector<ProcessInfo> clients, string name) {
    int index = 0;
    for (ProcessInfo client : clients) {
        if (client.processID == name) return index;
        index++;
    }
    return -1;
}

bool hasFile(ProcessInfo client, string file) {
    vector<string> files = client.files;
    for (string f : files) {
        if (f == file) return true;
    }
    return false;
}

// Find if a server has a file in a vector of ProcessInfo (servers)
ProcessInfo findFileServer(vector<ProcessInfo> clients, string file) {
    for (ProcessInfo client : clients) {
        if (hasFile(client, file)) return client;
    }
    throw "File not found on any active servers";
}

// Find the three servers having a file in a vector of ProcessInfo (servers)
vector<ProcessInfo> findFileServers(vector<ProcessInfo> clients, string file) {
    vector<ProcessInfo> set;
    for (ProcessInfo client : clients) {
        if (hasFile(client, file)) set.push_back(client);
    }
    if (set.empty()) throw "File not found on any active servers";
    return set;
}

// Make a tuple of server ids
string makeTuple(vector<ProcessInfo> set) {
    if (set.empty()) throw "Empty server list";
    string tuple = "";
    for (int i = 0; i < 3; i++) {
        if (i > 0)
            tuple += "-" + set[i].processID;
        else
            tuple += set[i].processID;
    }
    return tuple;
}

// Get the server IDs from the tuple of server IDs
vector<string> getFromTuple(string tuple) {
    vector<string> allfiles;
    istringstream line_stream(tuple);
    string file;
    while (getline(line_stream, file, '-')) {
        allfiles.push_back(file);
    }
    return allfiles;
}

// Check if all of the servers are dead
bool allDead(vector<ProcessInfo> set) {
    int deadCount = 0;
    for (ProcessInfo server : set) {
        if (!server.alive) {
            deadCount++;
        }
    }

    return deadCount == set.size();
}

template <class T>
T randomUnique(T begin, T end, size_t num_random) {
    size_t left = std::distance(begin, end);
    while (num_random--) {
        T r = begin;
        std::advance(r, rand() % left);
        std::swap(*begin, *r);
        ++begin;
        --left;
    }
    return begin;
}

// returns a random Process in the vector
ProcessInfo randomSelect(vector<ProcessInfo> set) {
    if (allDead(set)) throw "ALL SERVERS ARE DEAD";

    int randomIndex = rand() % set.size();
    if (set[randomIndex].alive)
        return set[randomIndex];
    else
        return randomSelect(set);
}

// returns a random Process in the vector
vector<ProcessInfo> randomSelectThree(vector<ProcessInfo> set) {
    if (allDead(set)) throw "ALL SERVERS ARE DEAD";

    vector<ProcessInfo> newSet;
    randomUnique(set.begin(), set.end(), 3);
    for (int i = 0; i < 3; i++) {
        newSet.push_back(set[i]);
    }

    return newSet;
}
