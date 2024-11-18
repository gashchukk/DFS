#include <string>
#include <vector>

class Client {
public:
    Client(const std::string& masterIP, int masterPort);
    void writeFile(const std::string& filename);

private:
    std::string masterIP;
    int masterPort;
    std::vector<std::string> getChunkServers(const std::string& filename); // Get chunk locations
};
