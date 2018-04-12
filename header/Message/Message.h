// Message.h
/*
    @author: antriksh
    Version 0: 3/14/2018
*/

class Message {
    /*
            Message: message being passed between two socket recipients.
    */

   public:
    string type;
    string message;
    int readWrite;
    int source;
    string sourceID;
    int destination;
    int timestamp;
    string fileName;
    int offset = 0;
    int byteCount = 0;

    Message() {}

    Message(int rw, string type, string m, int s, string sid, int d, int t)
        : message(m),
          type(type),
          readWrite(rw),
          source(s),
          sourceID(sid),
          destination(d),
          timestamp(t) {
        this->fileName = "NULL";
    }

    Message(int rw, string type, string m, int s, string sid, int d, int t,
            string f)
        : type(type),
          message(m),
          readWrite(rw),
          source(s),
          sourceID(sid),
          destination(d),
          timestamp(t),
          fileName(f) {}

    Message(int rw, string type, string m, int s, string sid, int d, int t,
            string f, int offset, int byteCount)
        : type(type),
          message(m),
          readWrite(rw),
          source(s),
          sourceID(sid),
          destination(d),
          timestamp(t),
          fileName(f),
          offset(offset),
          byteCount(byteCount) {}

    // CHECK
    bool operator<(const Message &rhs) const {
        /*
                for priority queue check of which request has a higher priority
        */
        if (this->timestamp < rhs.timestamp) {
            return true;
        } else if (this->timestamp == rhs.timestamp) {
            if (this->source < rhs.source) {
                return true;
            }
        }
        return false;
    }
};