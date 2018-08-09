// MetaInfo.hpp
/*
    @author: antriksh
    Version 0: 3/15/2018
*/

// Store Meta-Data for easy access
class MetaInfo {
   public:
    string name;
    string chunkName;
    string server;
    int queued;

    MetaInfo(string name, string chunkName, string server, int queued)
        : name(name), chunkName(chunkName), server(server), queued(queued) {}

    string getChunkFile() { return name + "_" + chunkName; }
};
