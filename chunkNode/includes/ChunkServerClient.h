// ChunkServerClient.h
#include <string>

class ChunkServerClient {
public:
    ChunkServerClient(const std::string& masterIP, int masterPort);
    void sendHeartbeat();

private:
    std::string masterIP;
    int masterPort;
};
