#include "ChunkServer.h"
#include <iostream>
#include <fstream>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <cstring>

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
    std::string filename = "chunks/" + chunkID; // Save chunks in a directory named "chunks"
    std::ofstream chunkFile(filename, std::ios::binary);
    if (chunkFile.is_open()) {
        chunkFile.write(data.c_str(), data.size());
        chunkFile.close();
        chunkStorage[chunkID] = filename;
        std::cout << "Stored chunk " << chunkID << " at " << filename << std::endl;
    } else {
        std::cerr << "Failed to store chunk " << chunkID << std::endl;
    }
}

std::string ChunkServer::retrieveChunk(const std::string& chunkID) {
    if (chunkStorage.find(chunkID) == chunkStorage.end()) {
        std::cerr << "Chunk " << chunkID << " not found" << std::endl;
        return "";
    }

    std::ifstream chunkFile(chunkStorage[chunkID], std::ios::binary);
    if (!chunkFile.is_open()) {
        std::cerr << "Failed to read chunk " << chunkID << std::endl;
        return "";
    }

    std::string data((std::istreambuf_iterator<char>(chunkFile)), std::istreambuf_iterator<char>());
    chunkFile.close();
    return data;
}

void ChunkServer::handleClientRequest(int clientSocket) {
    char buffer[256];
    bzero(buffer, 256);
    read(clientSocket, buffer, 255);

    std::string request(buffer);
    if (request.starts_with("STORE ")) {
        // Parse STORE command
        auto delimiterPos = request.find(" ");
        auto chunkIDPos = request.find(" ", delimiterPos + 1);
        std::string chunkID = request.substr(delimiterPos + 1, chunkIDPos - delimiterPos - 1);
        std::string data = request.substr(chunkIDPos + 1);

        storeChunk(chunkID, data);
    } else if (request.starts_with("RETRIEVE ")) {
        // Parse RETRIEVE command
        auto chunkID = request.substr(request.find(" ") + 1);

        std::string data = retrieveChunk(chunkID);
        send(clientSocket, data.c_str(), data.size(), 0);
    }
}
