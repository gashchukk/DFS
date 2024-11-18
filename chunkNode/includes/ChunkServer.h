#include <string>
#include <unordered_map>
#include <sys/stat.h>
#include <sys/types.h>
class ChunkServer {
public:
    ChunkServer(int port);
    void start();
    void storeChunk(const std::string& chunkID, const std::string& data);
    std::string retrieveChunk(const std::string& chunkID);

private:
    int serverPort;
    std::unordered_map<std::string, std::string> chunkStorage; // map of chunkID to chunk file path
    void handleClientRequest(int clientSocket);
};
