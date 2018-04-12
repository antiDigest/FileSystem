
/*
    @author: antriksh
    Version 1: 4/11/2018
*/

#include "../../utils.h"
#include "FileInfo.h"

// offset to to read in file
int getOffset(int size) { return size % CHUNKSIZE; }

// chunk where the information would be found
int getChunkNum(int size) { return size / CHUNKSIZE; }

// FileInfo to string
string readFileString(File* file) {
    return file->name + "," + to_string(file->size) + "," +
           to_string(file->chunks);
}

// Read and store information of all processes in a given fileName
vector<File*> readFileInfo(vector<File*> files, string fileName) {
    ifstream clientFile(fileName);
    string line;
    while (getline(clientFile, line)) {
        if (line.c_str()[0] == '#') continue;
        stringstream ss(line);
        string item;
        getline(ss, item, ',');
        string name = item;
        getline(ss, item, ',');
        long size = stoi(item);
        getline(ss, item, ',');
        int chunks = stoi(item);

        File* file = new File(name, size, chunks);
        files.push_back(file);
    }
    return files;
}

// Find an ID in a vector of clients/servers
File* findInVector(vector<File*> files, string name) {
    for (File* file : files) {
        if (file->name == name) return file;
    }
    return NULL;
}

// Read: reads the last line of the file and sends it to the process
string readFile(string INPUT_FILE) {
    string lastLine;
    ifstream file(INPUT_FILE);
    string line;
    while (getline(file, line) && line != "\n") {
        lastLine = line;
    }
    file.close();
    Logger("[RETURN READ FROM FILE]:" + lastLine);
    return lastLine;
}

// Read: reads from offset to bytecount length
string readFile(string INPUT_FILE, int offset, int bytecount) {
    char buffer[2048];
    ifstream file(INPUT_FILE);
    file.seekg(offset);
    file.read(buffer, bytecount);
    file.close();
    Logger("[RETURN READ FROM FILE]:" + string(buffer));
    return string(buffer);
}

// Write: Writes to the file and replies
void writeToFile(string INPUT_FILE, string message) {
    ofstream file;
    file.open(INPUT_FILE, ios::app);
    Logger("[WRITE TO FILE]:" + message);
    file << message << endl;
    file.close();
}

// returns a tuple from the vector of files
string makeFileTuple(vector<string> files) {
    string allfiles = "";
    for (string file : files) {
        if (file == "." || file == "..") continue;
        allfiles += file + ":";
    }
    return allfiles.substr(0, allfiles.length() - 1);
}

// returns a vector from a string of tuple containing all files
vector<string> getFiles(string files) {
    vector<string> allfiles;
    istringstream line_stream(files);
    string file;
    while (getline(line_stream, file, ':')) {
        allfiles.push_back(file);
    }
    return allfiles;
}

// returns a random file in the vector of files
string randomFileSelect(vector<string> files) {
    int randomIndex = rand() % files.size();
    return files[randomIndex];
}

// Enquiry: replies with a list of files
vector<string> readDirectory(string name) {
    vector<string> v;
    DIR* dirp = opendir(name.c_str());
    struct dirent* dp;
    while ((dp = readdir(dirp)) != NULL) {
        v.push_back(dp->d_name);
    }
    closedir(dirp);
    return v;
}

// Update Files information in csv
void updateCsv(string csv, vector<File*> files) {
    ofstream file;
    file.open(csv);
    file.close();

    for (File* file : files) {
        writeToFile(csv, readFileString(file));
    }
}
