
/*
    @author: antriksh
    Version 1: 4/11/2018
*/

#include "../includes.h"
#include "Message.h"

string messageString(Message *msg) {
    /*
            Converts message to tuple
    */
    return to_string(msg->readWrite) + ";" + msg->type + ";" + msg->message +
           ";" + to_string(msg->source) + ";" + msg->sourceID + ";" +
           to_string(msg->destination) + ";" + to_string(msg->timestamp) + ";" +
           msg->fileName + ";" + to_string(msg->offset) + ";" +
           to_string(msg->byteCount);
}

Message *getMessage(char *msg) {
    /*
            Converts char* tuple to Message
    */
    string message = msg;
    istringstream line_stream(message);
    string token;
    vector<string> tokens;
    while (getline(line_stream, token, ';')) {
        tokens.push_back(token);
    }

    int rw, s, t, d;
    string type = tokens[1];
    message = tokens[2];
    rw = stoi(tokens[0]);
    s = stoi(tokens[3]);
    string sid = tokens[4];
    d = stoi(tokens[5]);
    t = stoi(tokens[6]);
    string fileName = tokens[7];
    int offset = stoi(tokens[8]);
    int byteCount = stoi(tokens[9]);

    Message *m = new Message(rw, type, message, s, sid, d, t, fileName, offset,
                             byteCount);
    return m;
}
