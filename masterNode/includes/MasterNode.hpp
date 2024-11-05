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
    void createFile(const std::string& filename);
    void addChunk(const std::string& filename, const std::string& chunkID, const std::string& serverIP);
    std::vector<ChunkLocation> getChunkLocations(const std::string& filename);

private:
    std::unordered_map<std::string, std::vector<ChunkLocation>> fileMetadata; // maps file to chunk locations
};
