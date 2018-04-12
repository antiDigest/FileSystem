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

    MetaInfo(string name, string chunkName, string server)
        : name(name), chunkName(chunkName), server(server) {}

    string getChunkFile() { return name + "_" + chunkName; }
};
