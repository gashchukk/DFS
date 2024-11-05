#include <string>
#include "MasterNode.hpp"
class MasterNodeServer {
public:
    MasterNodeServer(int port);
    void start();

private:
    int serverPort;
    MasterNode masterNode;
    void handleConnection(int clientSocket);
};
