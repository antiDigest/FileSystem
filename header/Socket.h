// Socket.h
/*
    @author: antriksh
    Version 0: 3/14/2018
*/

#include "Info/File/utils.h"
#include "Info/Process/utils.h"
#include "Message/utils.h"

// Socket:
//     * Parent class to Server and Client
//     * Manages socket connections, send and receive
//     * stores general information of files, clients and servers
class Socket {
   protected:
    int clock = 0;
    string id;
    int personalfd;
    int port;
    string directory;

   public:
    int portno;
    socklen_t clilen;
    char *buffer;
    struct hostent *server;
    struct sockaddr_in serv_addr, cli_addr;
    int n = -1;
    queue<Message *> messageQueue;
    vector<ProcessInfo> allClients, allServers, mserver;
    vector<string> allFiles;

    // Initiates a socket connection
    // Finds the port(s) to connect to for the servers
    // initializes the list of clients and servers
    Socket(char *argv[]) {
        allClients = readClients(allClients, "csvs/clients.csv");
        allServers = readClients(allServers, "csvs/servers.csv");
        mserver = readClients(mserver, "csvs/mserver.csv");

        id = argv[1];
        logger.open("logs/" + id + ".txt", ios::out);

        portno = atoi(argv[2]);

        // Create a socket using socket() system call
        personalfd = socket(AF_INET, SOCK_STREAM, 0);
        if (personalfd < 0) error("ERROR opening socket");
        Logger("Socket connection established !");

        bzero((char *)&serv_addr, sizeof(serv_addr));
        serv_addr.sin_family = AF_INET;
        serv_addr.sin_addr.s_addr = INADDR_ANY;
        serv_addr.sin_port = htons(portno);

        struct in_addr ipAddr = serv_addr.sin_addr;
        char ipAddress[INET_ADDRSTRLEN];
        inet_ntop(AF_INET, &ipAddr, ipAddress, INET_ADDRSTRLEN);

        Logger("Process Address: " + string(ipAddress));
        Logger("Process ID: " + string(id));
        Logger("Process FD: " + to_string(personalfd));

        // bind the socket using the bind() system call
        if (::bind(personalfd, (struct sockaddr *)&serv_addr,
                   sizeof(serv_addr)) < 0)
            error("ERROR on binding");
        Logger("Bind complete !");

        // listen for connections using the listen() system call
        listen(personalfd, 5);
        clilen = sizeof(cli_addr);
        Logger("Listening for connections...");
    }

    // Connects to a hostname (ip address) at portno (port)
    // returns - file descriptor of socket stream connected to
    int connectTo(string hostname, int portno) {
        // Create a socket using socket() system call
        int fd = socket(AF_INET, SOCK_STREAM, 0);
        if (fd < 0) throw "ERROR opening socket";

        // Search for host
        struct hostent *host = gethostbyname(hostname.c_str());
        if (host == NULL) {
            throw "ERROR, no such host";
        }

        struct sockaddr_in host_addr, cli_addr;
        bzero((char *)&host_addr, sizeof(host_addr));
        host_addr.sin_family = AF_INET;
        bcopy((char *)host->h_addr, (char *)&host_addr.sin_addr.s_addr,
              host->h_length);
        host_addr.sin_port = htons(portno);

        // Attempting to connect to host
        if (connect(fd, (struct sockaddr *)&host_addr, sizeof(host_addr)) < 0)
            throw "ERROR connecting, the host may not be active";

        return fd;
    }

    // Send a message and log the send. Also updates system clock
    // @source - source file descriptor
    // @destination - destination file descriptor
    // @message - string to send
    // @destID - ID of the receiver (only for logging)
    // @rw - read/write/enquiry (default: none -- 0)
    // @filename - on request, the file to read/write (default: "NULL")
    void send(int source, int destination, string type, string message,
              string destID, int rw = 0, string filename = "NULL") {
        Message *msg = new Message(rw, type, message, source, id, destination,
                                   clock, filename);
        setClock();

        send(msg, destination, destID);
    }

    // Send a message and log the send
    // @m - already compiled Message
    // @fd - destination file descriptor
    // @destID - ID of the receiver (only for logging)
    void send(Message *m, int fd, string destID) {
        n = write(fd, messageString(m).c_str(), 2048);
        if (n < 0) error("ERROR writing to socket");

        Logger("[SENT TO " + destID + "]: " + m->message);
    }

    // Receive a message from the source (fd) and update clock using message
    // timestamp
    // @source - source file descriptor
    // returns - Message received
    Message *receive(int source) {
        char msg[2048];
        bzero(msg, 2048);
        n = recv(source, msg, 2048, MSG_WAITALL);
        if (n < 0) {
            error("ERROR reading from socket");
        } else if (n == 0) {
            error("Connection ended unexpectedly.");
        }

        Message *message = getMessage(msg);
        setClock(message->timestamp);

        Logger("[RECEIVED FROM " + message->sourceID +
               "]: " + message->message);

        return message;
    }

    // Send a message - quick send
    // @m - Message to be sent
    // @newsockfd - source file descriptor
    // @type - message type
    // @text - string to send
    void writeReply(Message *m, int newsockfd, string type, string text) {
        send(m->destination, newsockfd, type, text, m->sourceID, m->readWrite,
             m->fileName);
    }

    // Connect to a process and senc the message
    // @processID - process ID of the process to send to
    // @type - type of message
    // @rw - read/write
    // @fileName - name of file to read/write
    // @text - string to send
    void connectAndSend(string processID, string type, string text, int rw,
                        string fileName) {
        ProcessInfo p = findInVector(allServers, processID);
        int fd = connectTo(p.hostname, p.port);
        send(personalfd, fd, type, text, processID, rw, fileName);
        close(fd);
    }

    // Connect to a process and senc the message
    // @m - Message
    // @type - message type
    // @text - string to send
    void connectAndReply(Message *m, string type, string text) {
        ProcessInfo p = findInVector(allClients, m->sourceID);
        int fd = connectTo(p.hostname, p.port);
        m->type = type;
        m->message = text;
        m->sourceID = id;
        send(m, fd, p.processID);
        close(fd);
    }

    // Update clock
    void setClock() { clock += D; }

    // Update clock w.r.t received timestamp
    void setClock(int timestamp) { clock = max(clock + D, timestamp + D); }

    // RESET clock (not used)
    void resetClock() { clock = 0; }

    // Destructor
    ~Socket() { close(personalfd); }
};