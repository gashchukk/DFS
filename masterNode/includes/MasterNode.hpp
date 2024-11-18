#include <unordered_map>
#include <vector>
#include <string>

struct ChunkLocation {
    std::string chunkID;
    std::string serverIP; //this stores ips of servers where we keep each copy of this chunk
};

class MasterNode {
public:
    MasterNode();
    void createFile(const std::string& filename, const ChunkLocation& chunkLocation);
    std::string selectChunkServer();

private:
    std::unordered_map<std::string, std::vector<ChunkLocation>> fileMetadata; // maps file to chunk locations
    std::vector<std::string> availableChunkServers;
};
