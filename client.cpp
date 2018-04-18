/*
    @author: antriksh
    Version 0: 3/14/2018
    Version 0.1: 4/11/2018
        * Documentation updated
        * More structure to directories
        * #includes optimized
        * Ready for Version 0.1
    Version 1: Project 3 complete
*/

#include "header/Info/Meta/utils.h"
#include "header/Socket.h"

// Process: part of the lamport's mutual exclusion algorithm
// * to perform a read, gets meta-data of file from mServer and
// requests the related Server to read
// * to perform a write, gets meta-data of file from mServer and
// requests the related Server to append the line
// * Has a unique ID (given by the user)
class Process : public Socket {
   private:
    bool pendingEnquiry = false, pendingRead = false, pendingWrite = false;
    string pendingWriteMessage;
    int commitsRequired = 0;
    bool abortCommit = false;
    queue<string> commitMessages;
    vector<string> commitServers;

   public:
    // Enquires about the files in the system
    // sends requests for read and write of files to server while
    // maintaining mutual exclusion
    Process(char *argv[]) : Socket(argv) {}

    // Infinite thread to accept connection and detach a thread as
    // a receiver and checker of messages
    void listener() {
        while (1) {
            // Accept a connection with the accept() system call
            int newsockfd =
                accept(personalfd, (struct sockaddr *)&cli_addr, &clilen);
            if (newsockfd < 0) {
                error("ERROR on accept");
            }

            std::thread connectedThread(&Process::processMessages, this,
                                        newsockfd);
            connectedThread.detach();
        }
    }

    // Starts as a thread which receives a message and checks the message
    // @newsockfd - fd socket stream from which message would be received
    void processMessages(int newsockfd) {
        try {
            Message *message = receive(newsockfd);
            close(newsockfd);
            checkMessage(message, newsockfd);
        } catch (const char *e) {
            Logger(e);
            // break;
        }
    }

    // Checks the message for different types of incoming messages
    // 1. hi - just a hello from some client/server (not in use)
    // 2. meta - reply from the meta-server with resource meta-data
    // 3. reply - reply from a server to a request
    // 4. FAILED - failed response to a request
    // @m - Message just received
    // @newsockfd - socket stream it was received from
    void checkMessage(Message *m, int newsockfd) {
        // meta-data from the m-server
        if (m->type == "meta") {
            MetaInfo *meta = stringToInfo(m->message);
            m->fileName = meta->getChunkFile();
            cout << meta->server << endl;
            vector<string> threeServers = getFromTuple(meta->server);
            if (m->readWrite == 1) {
                ProcessInfo server = findInVector(allServers, threeServers[0]);
                readFromServer(m, server);
            } else if (m->readWrite == 2) {
                twoPhaseCommit(m, threeServers);
            }

            if (meta->queued == 0) {
                ProcessInfo p = mserver[0];
                int fd = connectTo(p.hostname, p.port);
                send(personalfd, fd, "inform", "", p.processID);
                close(fd);
            }

        } else if (m->type == "abort") {
            abortCommit = true;
            Logger("[TWO PHASE COMMIT]: [ABORT]");
            commitsRequired = 0;
            commitMessages.pop();
        } else if (m->type == "commit") {
            commitsRequired--;
            if (commitsRequired == 0) {
                Logger("[TWO PHASE COMMIT]: [COMMIT]");
                writeToServers(m, commitServers, commitMessages.front());
                commitMessages.pop();
            }
        } else if (m->type == "FAILED") {
            // TODO: write response from a server
            if (m->readWrite == 1) {
                pendingRead = false;
            } else {
                pendingWrite = false;
            }
        }

        throw "BREAKING CONNECTION";
    }

    // Enuiry - message to the meta-server for a list of files
    void enquiry() {
        ProcessInfo server = mserver[0];
        while (1) {
            try {
                Logger("Connecting to " + server.processID + " at " +
                       server.hostname);
                int fd = connectTo(server.hostname, server.port);
                send(personalfd, fd, "enquiry", "", server.processID, 3);
                close(fd);
                break;
            } catch (const char *e) {
                Logger(e);
            }
        }
    }

    // gets MetaData from the meta-server
    // requests the meta-server returned server for the resource
    void readRequest(string fileName, int offset, int byteCount) {
        int rw = 1;
        string message = "read request";
        getMetaData(rw, message, fileName, offset, byteCount);
    }

    // gets MetaData from the meta-server
    // requests the meta-server returned server for the resource
    void writeRequest(string fileName) {
        int rw = 2;

        cout << "Message (Enter/Return ends message): ";
        string content;
        getline(cin, content);
        getMetaData(rw, content, fileName);
    }

    // Section where the client corresponds with the server for read/write
    // @m - Message that was initially sent for request
    void writeToServers(Message *m, vector<string> threeServers,
                        string commitMessage) {
        string cs = "[WRITE TO SERVER]";
        int fd;
        for (string s : threeServers) {
            ProcessInfo server = findInVector(allServers, s);
            Logger(cs);

            fd = -1;
            while (fd < 0) {
                try {
                    fd = connectTo(server.hostname, server.port);
                } catch (const char *e) {
                    Logger(cs + e);
                    Logger(cs + "[FAILED]");
                    return;
                }
            }

            m->message = commitMessage;
            m->offset = getOffset(m->offset);
            send(m, fd, server.processID);
            Message *msg = receive(fd);
            Logger(cs + "[WRITE]" + m->fileName + "[LINE]" + msg->message);

            Logger(cs + "[EXIT]");
        }
    }

    // Section where the client corresponds with the server for read/write
    // @m - Message that was initially sent for request
    void readFromServer(Message *m, ProcessInfo server) {
        string cs = "[READ FROM SERVER]";
        Logger(cs);

        int fd = -1;
        while (fd < 0) {
            try {
                fd = connectTo(server.hostname, server.port);
            } catch (const char *e) {
                Logger(cs + e);
                Logger(cs + "[FAILED]");
                return;
            }
        }

        m->offset = getOffset(m->offset);
        send(m, fd, server.processID);
        Message *msg = receive(fd);
        Logger(cs + "[READ]" + m->fileName + "[LINE]" + msg->message);

        Logger(cs + "[EXIT]");
    }

    void twoPhaseCommit(Message *m, vector<string> threeServers) {
        Logger("[TWO PHASE COMMIT]");
        for (string server : threeServers) {
            connectAndSend(server, "twophase", m->message, 2, m->fileName);
            commitsRequired++;
        }
        commitServers = threeServers;
    }

    // Get Meta Data from the Meta-Server, send the meta-server a request for
    // meta-data
    // @rw - read/write request
    // @message - Message (for read, it is empty, for write it is the sentence
    // you want to write)
    // @fileName - file you would like to write to
    void getMetaData(int rw, string message, string fileName, int offset = 0,
                     int byteCount = 0) {
        ProcessInfo p = mserver[0];
        int fd = connectTo(p.hostname, p.port);
        Message *m = new Message(rw, "request", message, personalfd, id, fd,
                                 clock, fileName);
        if (offset > 0 || byteCount > 0) {
            m->offset = offset;
            m->byteCount = byteCount;
            send(m, fd, p.processID);
        } else {
            send(personalfd, fd, "request", message, p.processID, rw, fileName);
        }
        if (rw == 1) {
            pendingRead = true;
        } else {
            pendingWrite = true;
            commitMessages.push(m->message);
        }
        close(fd);
    }
};

// IO for continuous read and write messages
// @client - Process
void io(Process *client) {
    while (1) {
        cout << endl << "Welcome " + client->id + " $> ";
        string rw, file;
        cin >> rw;
        if (rw == "read") {
            cin >> file;
            int offset, byteCount;
            cin >> offset;
            cin >> byteCount;
            client->readRequest(file, offset, byteCount);
        } else if (rw == "write") {
            cin >> file;
            client->writeRequest(file);
        } else if (rw == "exit") {
            break;
        }
    }
}

int main(int argc, char *argv[]) {
    if (argc < 3) {
        fprintf(stderr, "usage %s ID port", argv[0]);
        exit(0);
    }

    // So that others can be activated
    // sleep(5);
    Process *client = new Process(argv);

    std::thread listenerThread(&Process::listener, client);
    io(client);
    listenerThread.join();

    logger.close();
    return 0;
}