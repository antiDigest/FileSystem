/*
    @author: antriksh
    Version 0: 3/14/2018
    Version 0.1: 4/11/2018
        * Documentation updated
        * More structure to directories
        * #includes optimized
        * Ready for Version 0.1
*/

#include "header/Info/Meta/utils.h"
#include "header/Socket.h"

class Mserver : public Socket {
    /*
        M-Server:
            * Manages files and file paths
            * Takes client requests to read and write
            * Replies to client with meta information
            * Asks random servers to create chunks of file
            * maintains information about all files
            * maintains and keeps updating information about all servers
            * Does not do anything for maintaining mutual exclusion

    */
   private:
    vector<File*> files;

   public:
    Mserver(char* argv[]) : Socket(argv) {
        files = readFileInfo(files, "csvs/files.csv");
    }

    // Infinite thread to accept connection and detach a thread as
    // a receiver and checker of messages
    void listener() {
        while (1) {
            // Accept a connection with the accept() system call
            int newsockfd =
                accept(personalfd, (struct sockaddr*)&cli_addr, &clilen);
            if (newsockfd < 0) {
                error("ERROR on accept");
            }

            std::thread connectedThread(&Mserver::processMessages, this,
                                        newsockfd);
            connectedThread.detach();
        }
    }

    // Starts as a thread which receives a message and checks the message
    // @newsockfd - fd socket stream from which message would be received
    void processMessages(int newsockfd) {
        try {
            Message* message = receive(newsockfd);
            close(newsockfd);
            checkMessage(message);
        } catch (const char* e) {
            Logger(e);
            close(newsockfd);
            // break;
        }
    }

    // Checks the message for different types of incoming messages
    // 1. heartbeat - just a hello from some server
    // 2. something else (means a request to be granted)
    // @m - Message just received
    // @newsockfd - socket stream it was received from
    void checkMessage(Message* m) {
        if (m->type == "heartbeat") {
            int index = findServerIndex(allServers, m->sourceID);
            registerHeartBeat(m, index);
        } else {
            checkReadWrite(m);
            throw "BREAKING CONNECTION";
        }
    }

    // Checks the request type
    // 1 - Read: replies with meta-data for the file
    // 2 - Write: replies with meta-data for the file
    // @m - Message just received
    // @newsockfd - socket stream it was received from
    void checkReadWrite(Message* m) {
        switch (m->readWrite) {
            case 1: {
                checkRead(m);
                break;
            }
            case 2: {
                checkWrite(m);
                break;
            }
            default: {
                string line = "UNRECOGNIZED MESSAGE !";
                connectAndReply(m, "", line);
                break;
            }
        }

        updateCsv("csvs/files.csv", files);
        Logger("Updated file info !");
    }

    // Checks what response should be given if the client wants to read a file
    void checkRead(Message* m) {
        try {
            File* file = findInVector(files, m->fileName);
            if (file == NULL) {
                connectAndReply(m, "FAILED", "FAILED");
            }

            string chunkName = to_string(getChunkNum(m->offset));
            string name = getChunkFile(m->fileName, chunkName);
            vector<ProcessInfo> threeServers =
                findFileServers(allServers, name);

            if (getOffset(m->offset) + m->byteCount > CHUNKSIZE) {
                readGreater(m, file, to_string(getChunkNum(m->offset)),
                            threeServers, m->offset, m->byteCount);
            } else {
                sizeSmaller(m, file, to_string(getChunkNum(m->offset)),
                            threeServers, m->offset, m->byteCount);
            }
        } catch (char* e) {
            Logger(e);
            Logger("[FAILED]");
            connectAndReply(m, "FAILED", "FAILED");
        }
    }

    // Checks what response should be given if the client wants to write to a
    // file
    void checkWrite(Message* m) {
        try {
            File* file;
            file = findInVector(files, m->fileName);
            if (file == NULL) {
                file = createNewFile(m, new File);
            }

            string chunkName = to_string(file->chunks - 1);
            string name = getChunkFile(file->name, chunkName);

            vector<ProcessInfo> threeServers =
                findFileServers(allServers, name);

            int messageSize = m->message.length();
            int chunkSize = getOffset(file->size);

            if (messageSize > (CHUNKSIZE - chunkSize)) {
                sizeGreater(m, file, chunkName, threeServers, chunkSize,
                            messageSize);
            } else {
                sizeSmaller(m, file, chunkName, threeServers, 0, messageSize);
            }
        } catch (char* e) {
            Logger(e);
            Logger("[FAILED]");
            connectAndReply(m, "FAILED", "FAILED");
        }
    }

    // Creates new file:
    // * adds a new file to the file infos
    // * creates a zero sized chunk of the file at a random server
    // * returns the FileInfo
    File* createNewFile(Message* m, File* file) {
        Logger("[Creating new file]: " + m->fileName);
        file->name = m->fileName;
        createNewChunk(file, to_string(file->chunks));
        files.push_back(file);

        return file;
    }

    // When size of the write message size + size of the chunk is greater than
    // 8192 bytes.
    // The message is divided into two parts. The first part completes
    // 8192 bytes on the last available chunk and the second part is written to
    // a newly created chunk
    void readGreater(Message* m, File* file, string chunkName,
                     vector<ProcessInfo> server, int offset, int byteCount) {
        int sizeThisChunk = byteCount - (CHUNKSIZE - offset);
        int extraByteCount = byteCount - sizeThisChunk;

        string source = m->sourceID;
        sizeSmaller(m, file, chunkName, server, offset, sizeThisChunk);
        chunkName = to_string(stoi(chunkName) + 1);
        vector<ProcessInfo> threeServers;
        try {
            threeServers = findFileServers(allServers,
                                           getChunkFile(file->name, chunkName));
        } catch (char* e) {
            connectAndReply(m, "FAILED", "FAILED");
            return;
        }

        m->sourceID = source;  // TODO: I should not have to do this
        sizeSmaller(m, file, chunkName, threeServers, 0, extraByteCount);
    }

    // When size of the write message size + size of the chunk is greater than
    // 8192 bytes.
    // The message is divided into two parts. The first part completes
    // 8192 bytes on the last available chunk and the second part is written to
    // a newly created chunk
    void sizeGreater(Message* m, File* file, string chunkName,
                     vector<ProcessInfo> server, int chunkSize,
                     int messageSize) {
        int sizeThisChunk = messageSize - (CHUNKSIZE - chunkSize);
        int sizeNewChunk = messageSize - sizeThisChunk;

        sizeSmaller(m, file, chunkName, server, 0, sizeThisChunk);

        chunkName = to_string(stoi(chunkName) + 1);
        createNewChunk(file, chunkName);
        vector<ProcessInfo> threeServers;
        try {
            threeServers = findFileServers(allServers,
                                           getChunkFile(file->name, chunkName));
        } catch (char* e) {
            connectAndReply(m, "FAILED", "FAILED");
            return;
        }
        sizeSmaller(m, file, chunkName, threeServers, sizeThisChunk,
                    sizeNewChunk);
    }

    // When size of the write message size + size of the chunk is less than
    // 8192 bytes.
    // The message is written to the last available chunk of the file.
    void sizeSmaller(Message* m, File* file, string chunkName,
                     vector<ProcessInfo> servers, int offset, int byteCount) {
        m->offset = offset;
        m->byteCount = byteCount;
        replyMeta(m, file->name, chunkName, servers);
    }

    // reply with the meta information of reading or writing to a file. The
    // meta-server replies with the chunk number, the server the file is on, and
    // the file name
    void replyMeta(Message* m, string fileName, string chunkName,
                   vector<ProcessInfo> servers) {
        vector<ProcessInfo> set;
        for (ProcessInfo server : servers) {
            if (server.getReady())
                set.push_back(server);
            else {
                int index = findServerIndex(allServers, server.processID);
                Logger("[SERVER DOWN]: " + allServers[index].processID +
                       " needs to update chunk " +
                       getChunkFile(fileName, chunkName));
                // cout << getChunkFile(fileName, chunkName) << endl;
                allServers[index].chunksNeedUpdate.insert(
                    getChunkFile(fileName, chunkName));
            }
        }

        MetaInfo* meta = new MetaInfo(fileName, chunkName, makeTuple(set));
        string line = infoToString(meta);

        if (!set.empty()) {
            connectAndReply(m, "meta", line);
        } else {
            connectAndReply(m, "FAILED", "FAILED");
        }
    }

    // Creating new chunk:
    // * randomly selects a server.
    // * connects to the server and tells it to create a new chunk
    // * updates meta data about the server
    void createNewChunk(File* file, string chunkName) {
        int selected = 0;
        vector<ProcessInfo> threeServers = randomSelectThree(allServers);
        file->chunks++;
        for (ProcessInfo server : threeServers) {
            Logger("[Creating new file chunk at]: " + server.processID);
            string fileName = getChunkFile(file->name, chunkName);
            connectAndSend(server.processID, "create", "", 2, fileName);
            updateMetaData(file, server);
        }
    }

    // Updating meta-data in the meta Directory
    void updateMetaData(File* file, ProcessInfo server, int msgSize = 0) {
        server.addFile(getChunkFile(file->name, to_string(file->chunks)));
        file->size += msgSize;
        Logger("[Meta-Data Updated]");
        updateServers(server);
    }

    // Update information about servers
    void updateServers(ProcessInfo server) {
        int index = 0;
        for (ProcessInfo s : allServers) {
            if (s.processID == server.processID) break;
            index++;
        }
        allServers.at(index) = server;
    }

    // Handle the case when a chunk comes back alive after being dead for some
    // time
    void recoveringServer(int index) {
        if (!allServers[index].chunksNeedUpdate.empty())
            for (string chunk : allServers[index].chunksNeedUpdate) {
                Logger("[RECOVERING]: Updating file: " + chunk);

                // Connecting to server which was down
                ProcessInfo p = allServers[index];
                cout << p.processID << endl;
                int fdBroken = connectTo(p.hostname, p.port);
                send(personalfd, fdBroken, "head", "", id, 2, chunk);
                Message* msg = receive(fdBroken);

                vector<ProcessInfo> others = findFileServers(allServers, chunk);
                for (ProcessInfo another : others) {
                    if (another.processID == p.processID || !another.getReady())
                        continue;
                    // Connecting to a server which was alive
                    cout << another.processID << endl;
                    int fdUpdated = connectTo(another.hostname, another.port);
                    send(personalfd, fdUpdated, "recover", msg->message, id, 2,
                         chunk);
                    msg = receive(fdUpdated);
                    close(fdUpdated);
                    break;
                }
                send(personalfd, fdBroken, "update", msg->message, id, 2,
                     chunk);
            }
        else {
            Logger("[RECOVERING]: No chunks need an update.");
        }
        allServers[index].setReady();
    }

    // Register the read heartbeat
    // * Updates the alive time at the Server info
    void registerHeartBeat(Message* m, int index) {
        Logger("[HEARTBEAT] " + m->sourceID, false);
        if (allServers[index].getAlive()) {
            allServers[index].setAlive();
            allServers[index].updateFiles(m->message);
        } else {
            allServers[index].setAlive();
            allServers[index].updateFiles(m->message);
            Logger("[RECOVERING]: " + m->sourceID);
            recoveringServer(index);
        }
    }

    // Continuous thread to check if which server is not alive.
    // * checks seconds from last alive time from the server info
    // * declares as dead if last alive was 15 seconds ago.
    void serversAlive() {
        while (1) {
            for (int i = 0; i < allServers.size(); i++) {
                allServers[i].checkAlive();
            }
            sleep(1);
        }
    }

    ~Mserver() { updateCsv("csvs/files.csv", files); }
};

// IO for continuous read and write messages
// @client - Process
void io(Mserver* mserver) {
    int rw = 25;
    int i = 0;
    sleep(rw);
    for (ProcessInfo server : mserver->allServers) {
        cout << server.getAlive() << endl;
    }
}

int main(int argc, char* argv[]) {
    if (argc < 3) {
        fprintf(stderr, "usage %s ID port\n", argv[0]);
        exit(1);
    }

    Mserver* server = new Mserver(argv);

    std::thread listenerThread(&Mserver::listener, server);
    std::thread countDown(&Mserver::serversAlive, server);
    countDown.join();
    listenerThread.join();

    logger.close();
    return 0;
}