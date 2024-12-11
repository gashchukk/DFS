#ifndef CHUNK_SERVER_CLIENT_H
#define CHUNK_SERVER_CLIENT_H

#include <string>
#include "ChunkServer.h" 

class ChunkServerClient {
public:
    ChunkServerClient(const std::string& masterIP, int masterPort, const ChunkServer& server);

    void sendHeartbeat(const std::string& chunkName);
    const ChunkServer& chunkServer; 

private:
    std::string masterIP;  
    int masterPort;         
};

#endif // CHUNK_SERVER_CLIENT_H
