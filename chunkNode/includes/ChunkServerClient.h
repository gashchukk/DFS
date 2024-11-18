#ifndef CHUNK_SERVER_CLIENT_H
#define CHUNK_SERVER_CLIENT_H

#include <string>

class ChunkServerClient {
public:
    // Constructor to initialize with Master Node IP and port
    ChunkServerClient(const std::string& masterIP, int masterPort);

    // Sends a heartbeat message with the chunk name
    void sendHeartbeat(const std::string& chunkName);

private:
    std::string masterIP;   // IP address of the Master Node
    int masterPort;         // Port of the Master Node
};

#endif // CHUNK_SERVER_CLIENT_H
