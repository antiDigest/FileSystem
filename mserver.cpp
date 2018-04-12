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
    }

    // Checks what response should be given if the client wants to read a file
    void checkRead(Message* m) {
        try {
            string chunkName = to_string(getChunkNum(m->offset));
            string name = getChunkFile(m->fileName, chunkName);
            ProcessInfo server = findFileServer(allServers, name);
            replyMeta(m, m->fileName, chunkName, server);
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

            ProcessInfo server = findFileServer(allServers, name);

            int messageSize = m->message.length();
            int chunkSize = getOffset(file->size);

            if (messageSize > (CHUNKSIZE - chunkSize)) {
                sizeGreater(m, file, chunkName, server, chunkSize, messageSize);
            } else {
                sizeSmaller(m, file, chunkName, server, 0, messageSize);
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
    void sizeGreater(Message* m, File* file, string chunkName,
                     ProcessInfo server, int chunkSize, int messageSize) {
        int sizeThisChunk = messageSize - (CHUNKSIZE - chunkSize);
        int sizeNewChunk = messageSize - sizeThisChunk;

        sizeSmaller(m, file, chunkName, server, 0, sizeThisChunk);

        chunkName = to_string(stoi(chunkName) + 1);
        ProcessInfo newServer = createNewChunk(file, chunkName, sizeNewChunk);
        sizeSmaller(m, file, chunkName, newServer, sizeThisChunk, sizeNewChunk);
    }

    // When size of the write message size + size of the chunk is less than
    // 8192 bytes.
    // The message is written to the last available chunk of the file.
    void sizeSmaller(Message* m, File* file, string chunkName,
                     ProcessInfo server, int offset, int byteCount) {
        m->offset = offset;
        m->byteCount = byteCount;
        replyMeta(m, file->name, chunkName, server);
    }

    // reply with the meta information of reading or writing to a file. The
    // meta-server replies with the chunk number, the server the file is on, and
    // the file name
    void replyMeta(Message* m, string fileName, string chunkName,
                   ProcessInfo server) {
        MetaInfo* meta = new MetaInfo(fileName, chunkName, server.processID);
        string line = infoToString(meta);

        if (server.getAlive()) {
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
        ProcessInfo server = randomSelect(allServers);
        Logger("[Creating new file chunk at]: " + server.processID);
        string fileName = getChunkFile(file->name, chunkName);
        connectAndSend(server.processID, "create", "", 2, fileName);
        updateMetaData(file, server);
    }

    // Creating new chunk:
    // * randomly selects a server.
    // * connects to the server and tells it to create a new chunk
    // * updates meta data about the server
    // * returns the server id to which the chunk was created
    ProcessInfo createNewChunk(File* file, string chunkName, int msgSize) {
        ProcessInfo server = randomSelect(allServers);
        Logger("[Creating new file chunk at]: " + server.processID);
        string fileName = getChunkFile(file->name, chunkName);
        connectAndSend(server.processID, "create", "", 2, fileName);
        updateMetaData(file, server, msgSize);
        return server;
    }

    // Updating meta-data in the meta Directory
    void updateMetaData(File* file, ProcessInfo server, int msgSize = 0) {
        server.addFile(getChunkFile(file->name, to_string(file->chunks)));
        file->chunks++;
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

    // Register the read heartbeat
    // * Updates the alive time at the Server info
    void registerHeartBeat(Message* m, int index) {
        Logger("[HEARTBEAT] " + m->sourceID);
        allServers[index].setAlive();
        allServers[index].updateFiles(m->message);
    }

    // Continuous thread to check if which server is not alive.
    // * checks seconds from last alive time from the server info
    // * declares as dead if last alive was 15 seconds ago.
    void serversAlive() {
        while (1) {
            for (int i = 0; i < allServers.size(); i++) {
                allServers[i].checkAlive();
            }
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
    if (argc < 4) {
        fprintf(stderr, "usage %s ID port directory_path\n", argv[0]);
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