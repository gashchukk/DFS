#include <unordered_map>
#include <vector>
#include <string>

struct ChunkLocation {
    std::string chunkID;
    std::vector<std::string> serverIPs; //this stores ips of servers where we keep each copy of this chunk
};

class MasterNode {
public:
    MasterNode();
    void createFile(const std::string& filename, std::string& chunkServerIP);
    std::string selectChunkServer();
private:
    std::unordered_map<std::string, std::vector<ChunkLocation>> fileMetadata; // maps file to chunk locations
    std::vector<std::string> availableChunkServers;
};
