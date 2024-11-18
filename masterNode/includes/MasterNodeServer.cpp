// MasterNodeServer.cpp
#include "MasterNodeServer.hpp"

#include <iostream>
#include <netinet/in.h>
#include <unistd.h>
#include <cstring>
#include <iostream>
#include <string>
#include <thread>

MasterNodeServer::MasterNodeServer(int port) : serverPort(port) {}

std::unordered_map<std::string, std::chrono::time_point<std::chrono::steady_clock>> activeChunkServers;

void MasterNodeServer::start() {
    int serverSocket = socket(AF_INET, SOCK_STREAM, 0);
    if (serverSocket < 0) {
        std::cerr << "Error creating socket" << std::endl;
        return;
    }

    struct sockaddr_in serverAddr;
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_addr.s_addr = INADDR_ANY;
    serverAddr.sin_port = htons(serverPort);

    if (bind(serverSocket, (struct sockaddr*)&serverAddr, sizeof(serverAddr)) < 0) {
        std::cerr << "Binding failed" << std::endl;
        return;
    }

    listen(serverSocket, 5);
    std::cout << "Master Node listening on port " << serverPort << std::endl;

    while (true) {
        int clientSocket = accept(serverSocket, nullptr, nullptr);
        if (clientSocket >= 0) {
            std::cout << "Client or Chunk Server connected" << std::endl;
            handleConnection(clientSocket);
            close(clientSocket);
        }
    }
}



void MasterNodeServer::handleConnection(int clientSocket) {
    char buffer[1024];
    int bytesReceived = recv(clientSocket, buffer, sizeof(buffer), 0);
    if (bytesReceived <= 0) {
        std::cerr << "Error receiving data from client" << std::endl;
        return;
    }

    std::string request(buffer, bytesReceived);
    std::string response;
    std::string chunkServerIP;
    if(request.starts_with("GET_CHUNK_LOCATION")) {
        try{
            chunkServerIP = masterNode.selectChunkServer();
            response = "ChunkServerIP: " + chunkServerIP + "\n";
            send(clientSocket, response.c_str(), response.size(), 0);
        }catch(const std::exception& e) {
            response = std::string("Error: ") + e.what() + "\n";
        }
    }

if (request.starts_with("HEARTBEAT:")) { // chunkserver request
    size_t ipStart = request.find(':') + 1;
    size_t chunkStart = request.find(':', ipStart) + 1;

    std::string ip = request.substr(ipStart, chunkStart - ipStart - 1);
    std::string chunkName = request.substr(chunkStart);

    // Derive filename from chunkName (remove the last part after the underscore)
    size_t underscorePos = chunkName.rfind('_');
    std::string filename = chunkName.substr(0, underscorePos);

    // Process the heartbeat (e.g., update metadata)
    std::cout << "Received heartbeat from " << ip << " for chunk " << chunkName << " (filename: " << filename << ")" << std::endl;

    ChunkLocation chunk(chunkName, ip);
    masterNode.createFile(filename, chunk);
}

    }
