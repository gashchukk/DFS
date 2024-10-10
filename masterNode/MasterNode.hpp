#ifndef MASTERNODE_HPP
#define MASTERNODE_HPP

#include <iostream>
#include <unordered_map>
#include <vector>
#include <string>
#include <mutex>
#include <ctime>
#include <netinet/in.h>

// MasterNode class definition
class MasterNode {
public:
    MasterNode();  // Constructor

    // Register a new chunk server
    void registerChunkServer(const std::string& serverId);

    // Update heartbeat of a server
    void updateHeartbeat(const std::string& serverId);

    // Allocate a chunk for a file
    void allocateChunk(const std::string& fileName);

    // Get chunk location for a given file
    std::pair<std::string, int> getChunkLocation(const std::string& fileName);

    // Monitor chunk servers for heartbeat (runs in a separate thread)
    void monitorChunkServers();

    // Start the server to accept connections from Data Nodes
    void startMasterServer(int port);

private:
    // Structure to store information about chunk servers
    struct ChunkServerInfo {
        std::string status;
        std::time_t lastHeartbeat;
        std::vector<int> chunks;
    };

    // Chunk server map (server ID -> ChunkServerInfo)
    std::unordered_map<std::string, ChunkServerInfo> chunkServers_;

    // Metadata for file-to-chunk mapping (file name -> chunk ID)
    std::unordered_map<std::string, int> metadata_;

    // Mutex for thread-safe operations
    std::mutex mutex_;

    // Function to handle each client connection (data node)
    void handleClient(int clientSocket);
};

#endif  // MASTERNODE_HPP
