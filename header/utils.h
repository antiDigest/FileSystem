// utils.h
/*
    @author: antriksh
    Version 0: 3/14/2018
*/

#include "includes.h"

using namespace std;

string logfile;
ofstream logger;

// returns a string of timestamp
string globalTime() {
    time_t now = time(0);  // get time now
    struct tm tstruct;
    char buf[80];
    tstruct = *localtime(&now);
    strftime(buf, sizeof(buf), "%Y-%m-%d.%X", &tstruct);

    return buf;
}
// Logs to a file and stdout
// @message - string to log
void Logger(string message) {
    logger << "[" << globalTime() << "]::" << message << endl;
    if (message != "[HEARTBEAT]")
        cout << "[" << globalTime() << "]::" << message << endl;
    logger.flush();
    return;
}

// Logs and shows error messages
void error(const char *msg) {
    Logger(msg);
    exit(1);
}