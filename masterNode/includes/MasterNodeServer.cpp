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
const int HEARTBEAT_TIMEOUT = 10;

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

    std::thread([&]() {
        while (true) {
            // Monitor for inactive Chunk Servers and remove it
            auto now = std::chrono::steady_clock::now();
            for (auto it = activeChunkServers.begin(); it != activeChunkServers.end(); ) {
                auto duration = std::chrono::duration_cast<std::chrono::seconds>(now - it->second).count();
                if (duration > HEARTBEAT_TIMEOUT) {
                    std::cout << "Chunk Server " << it->first << " is inactive" << std::endl;
                    it = activeChunkServers.erase(it); 
                } else {
                    ++it;
                }
            }
            std::this_thread::sleep_for(std::chrono::seconds(HEARTBEAT_TIMEOUT / 2));
        }
    }).detach();

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

    if (request.starts_with("HEARTBEAT:")) {
        std::string chunkServerIP = request.substr(10); 
        activeChunkServers[chunkServerIP] = std::chrono::steady_clock::now();
        std::cout << "Received heartbeat from ChunkServer: " << chunkServerIP << std::endl;
        response = "Heartbeat received\n";

    } else if (request.starts_with("CREATE_FILE")) {
        std::string filename = request.substr(12);
        masterNode.createFile(filename);
        response = "File created successfully\n";

    } else if (request.starts_with("ADD_CHUNK")) {
        size_t pos = request.find(" ");
        std::string filename = request.substr(pos + 1, request.find(" ", pos + 1) - pos - 1);
        std::string chunkID = request.substr(request.find(" ", pos + 1) + 1);

        std::string serverIP = "127.0.0.1"; 
        masterNode.addChunk(filename, chunkID, serverIP);
        response = "Chunk added successfully\n";

    } else if (request.starts_with("GET_CHUNK_LOCATIONS")) {
        std::string filename = request.substr(19);
        try {
            auto chunkLocations = masterNode.getChunkLocations(filename);
            response = "Chunk locations:\n";
            for (const auto& location : chunkLocations) {
                response += location.chunkID + " on server(s): ";
                for (const auto& ip : location.serverIPs) {
                    response += ip + " ";
                }
                response += "\n";
            }
        } catch (const std::exception& e) {
            response = std::string("Error: ") + e.what() + "\n";
        }
    } else {
        response = "Unknown command\n";
    }

    send(clientSocket, response.c_str(), response.size(), 0);
}
