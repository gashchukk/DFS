#ifndef MASTERNODE_HPP
#define MASTERNODE_HPP

#include <iostream>
#include <unordered_map>
#include <vector>
#include <string>
#include <mutex>
#include <ctime>
#include <netinet/in.h>

class MasterNode
{
public:
    MasterNode();
    void registerChunkServer(const std::string &serverId);
    void updateHeartbeat(const std::string &serverId);
    void allocateChunk(const std::string &fileName);
    std::pair<std::string, int> getChunkLocation(const std::string &fileName);
    void monitorChunkServers();
    void startMasterServer(int port);

private:
    struct ChunkServerInfo
    {
        std::string status;
        std::time_t lastHeartbeat;
        std::vector<int> chunks;
    };
    std::unordered_map<std::string, ChunkServerInfo> chunkServers_;
    std::unordered_map<std::string, int> metadata_;
    std::mutex mutex_;
    void handleClient(int clientSocket);
};

#endif // MASTERNODE_HPP
