#include "ChunkServerClient.h"
#include <iostream>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <cstring>

ChunkServerClient::ChunkServerClient(const std::string& ip, int port)
    : masterIP(ip), masterPort(port) {}

void ChunkServerClient::sendHeartbeat() {
    int sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0) {
        std::cerr << "Socket creation error" << std::endl;
        return;
    }

    struct sockaddr_in serverAddr;
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(masterPort);

    if (inet_pton(AF_INET, masterIP.c_str(), &serverAddr.sin_addr) <= 0) {
        std::cerr << "Invalid address/ Address not supported" << std::endl;
        return;
    }

    if (connect(sock, (struct sockaddr*)&serverAddr, sizeof(serverAddr)) < 0) {
        std::cerr << "Connection Failed" << std::endl;
        return;
    }

    // Include the IP address of the ChunkServer in the heartbeat message
    std::string heartbeatMsg = "HEARTBEAT:" + masterIP;
    send(sock, heartbeatMsg.c_str(), heartbeatMsg.size(), 0);
    std::cout << "Heartbeat sent to Master" << std::endl;

    close(sock);
}
