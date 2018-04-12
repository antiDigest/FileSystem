// ProcessInfo.hpp
/*
    @author: antriksh
    Version 0: 3/14/2018
*/

// Store information of a process
class ProcessInfo {
   public:
    int fd;
    string processID;
    string hostname;
    int port;
    string system;
    bool repliedRead;
    bool repliedWrite;
    time_t aliveTime = time(0);
    bool alive = true;
    vector<string> files;

    void setAlive() {
        alive = true;
        aliveTime = time(0);
    }

    void checkAlive() {
        float aliveLast = (time(0) - getAliveTime());
        if (alive && aliveLast > 15.0) {
            Logger("[" + processID + " has missed 3 heartbeats]");
            resetAlive();
        }
    }

    void resetAlive() { alive = false; }

    bool getAlive() { return alive; }

    time_t getAliveTime() { return aliveTime; }

    void addFile(string name) { files.push_back(name); }

    void updateFiles(string f) {
        string filestring = makeFileTuple(files);
        if (filestring != f) {
            files = getFiles(f);
        }
    }
};
