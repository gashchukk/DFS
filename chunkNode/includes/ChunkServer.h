#ifndef CHUNK_SERVER_H
#define CHUNK_SERVER_H

#include <string>
#include <unordered_map>
#include <sys/stat.h>
#include <sys/types.h>

class ChunkServer {
public:
    ChunkServer(int port);
    void start();
    void storeChunk( std::string& chunkID, const std::string& data);
    void storeChunkCopies( std::string& chunkName, const std::vector<std::string>& servers);

    int getServerPort() const; 

private:
    int serverPort;
    std::unordered_map<std::string, std::string> chunkStorage; // map of chunkID to chunk file path
    void handleClientRequest(int clientSocket);
    void notifyMaster();
};

// Implementation


#endif // CHUNK_SERVER_H
