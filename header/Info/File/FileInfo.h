// utils.h
/*
    @author: antriksh
    Version 0: 3/20/2018
*/

class File {
   public:
    string name;
    long size = 0;
    int chunks = 0;

    File() {}

    File(string name, long size, int chunks)
        : name(name), size(size), chunks(chunks) {}
};
