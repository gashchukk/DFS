#ifndef MASTERNODE_HPP
#define MASTERNODE_HPP

#include <iostream>
#include <unordered_map>
#include <vector>
#include <string>
#include <mutex>
#include <ctime>
#include <netinet/in.h>

enum class Status {
    ALIVE = 0x1,
    DEAD = 0x2,
};


struct ChunkServerInfo {
    Status status;
    std::time_t lastHeartbeat;
    std::vector<int> chunks;
    int port;
};


class MasterNode {
public:
    MasterNode();
    
    void registerChunkServer(const std::string& serverId, int port);
    void updateHeartbeat(const std::string& serverId);
    void allocateChunk(const std::string& fileName);
    std::pair<std::string, int> getChunkLocation(const std::string& fileName);
    void monitorChunkServers();
    void handleClient(int clientSocket);
    void startMasterServer(int port);


private:
    std::unordered_map<std::string, ChunkServerInfo> chunkServers_;
    std::unordered_map<std::string, int> metadata_;
    std::mutex mutex_;
    int numDataNodes = 3;  // Assume we have 3 data nodes
};


#endif // MASTERNODE_HPP
