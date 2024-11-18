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

    if (request.starts_with("HEARTBEAT:")) { //chunkserver request
        chunkServerIP = request.substr(10); 
        activeChunkServers[chunkServerIP] = std::chrono::steady_clock::now();
        std::cout << "Received heartbeat from ChunkServer: " << chunkServerIP << std::endl;
        response = "Heartbeat received\n";

    } else if (request.starts_with("CREATE_FILE")) { // client request
        std::string filename = request.substr(12);
        masterNode.createFile(filename, chunkServerIP);
            

    }else {
            response = "Unknown command\n";
        }

}
