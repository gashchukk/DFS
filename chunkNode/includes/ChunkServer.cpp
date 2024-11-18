#include "ChunkServer.h"
#include <iostream>
#include <fstream>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <cstring>
#include <sys/stat.h>
#include <sys/types.h>
#include <filesystem>
#include <regex>
#include <vector>
#include <algorithm>
#include <sstream>

ChunkServer::ChunkServer(int port) : serverPort(port) {}

void ChunkServer::start() {
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
    std::cout << "Chunk Server listening on port " << serverPort << std::endl;


    while (true) {
        int clientSocket = accept(serverSocket, nullptr, nullptr);
        if (clientSocket >= 0) {
            handleClientRequest(clientSocket);
            close(clientSocket);
        }
    }
}

void ChunkServer::storeChunk(const std::string& chunkID, const std::string& data) {
    std::string filename = "chunks/" + chunkID; 

    std::ofstream chunkFile(filename);
    if (chunkFile.is_open()) {
        chunkFile.write(data.c_str(), data.size());
        chunkFile.close();
        chunkStorage[chunkID] = filename;
        std::cout << "Stored chunk " << chunkID << " at " << filename << std::endl;
    } else {
        std::cerr << "Failed to store chunk " << chunkID << std::endl;
    }
}

void ChunkServer::handleClientRequest(int clientSocket) {
    char buffer[256];
    bzero(buffer, 256);
    read(clientSocket, buffer, 255);

    std::string request(buffer);

    if (request.starts_with("STORE ")) {
        std::cout << "Received request: STORE "<< std::endl;
        auto delimiterPos = request.find(" ");
        auto chunkIDPos = request.find(" ", delimiterPos + 1);
        std::string chunkID = request.substr(delimiterPos + 1, chunkIDPos - delimiterPos - 1);
        std::string data = request.substr(chunkIDPos + 1);

        storeChunk(chunkID, data);
    } else if (request.starts_with("RETRIEVE ")) {
    std::cout << "Received request: RETRIEVE "  << std::endl;

    std::string chunkName = request.substr(9); 
    std::string chunkPath = "chunks/" + chunkName;

    std::ifstream chunkFile(chunkPath, std::ios::binary);
    if (chunkFile) {
        std::cout << "Found chunk: " << chunkName << std::endl;

        std::stringstream buffer;
        buffer << chunkFile.rdbuf();

        std::string chunkData = buffer.str();

        send(clientSocket, chunkData.c_str(), chunkData.length(), 0);

        std::cout << "Sent chunk data for: " << chunkName << std::endl;
    } else {
        std::cerr << "Chunk not found: " << chunkName << std::endl;

        std::string errorMessage = "ERROR: Chunk " + chunkName + " not found.";
        send(clientSocket, errorMessage.c_str(), errorMessage.length(), 0);
    }
}
 else {
        std::string errorMsg = "ERROR: Invalid command\n";
        write(clientSocket, errorMsg.c_str(), errorMsg.size());
        std::cerr << "Invalid command received." << std::endl;
    }
}
