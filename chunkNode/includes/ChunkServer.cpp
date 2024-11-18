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
    std::string directory = "chunks";
    struct stat info;

    if (stat(directory.c_str(), &info) != 0 || !(info.st_mode & S_IFDIR)) {
        // Directory does not exist, create it
        if (mkdir(directory.c_str(), 0777) != 0) {
            std::cerr << "Failed to create directory: " << directory << std::endl;
            return;
        }
    }

    std::string filename = directory + "/" + chunkID; // Save chunks in "chunks"
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
    std::cout << "Received request: " << request << std::endl;

    if (request.starts_with("STORE ")) {
        // Parse STORE command
        auto delimiterPos = request.find(" ");
        auto chunkIDPos = request.find(" ", delimiterPos + 1);
        std::string chunkID = request.substr(delimiterPos + 1, chunkIDPos - delimiterPos - 1);
        std::string data = request.substr(chunkIDPos + 1);

        storeChunk(chunkID, data);
    } else if (request.starts_with("RETRIEVE ")) {
       std::cout<<"retrieve";
    } else {
        std::string errorMsg = "ERROR: Invalid command\n";
        write(clientSocket, errorMsg.c_str(), errorMsg.size());
        std::cerr << "Invalid command received." << std::endl;
    }
}

