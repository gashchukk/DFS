#include <unordered_map>
#include <vector>
#include <string>

struct ChunkLocation {
    std::string chunkID;
    std::string serverIP; 
};

class MasterNode {
public:
    MasterNode();
    void createFile(const std::string& filename, const ChunkLocation& chunkLocation);
    void printFileMetadata();
    std::string selectChunkServer();
    std::vector<std::pair<std::string, std::string>> readFileRequest(const std::string& request);
     std::vector<std::string> selectTwoDifferentServers(std::string& currentServer);
    void sendSelectedServersToCurrentServer(std::string& currentServer, std::string& chunkName);
    
    
    std::unordered_map<std::string, std::vector<ChunkLocation>> fileMetadata; // maps file to chunk locations
    std::vector<std::string> availableChunkServers;

private:
};
