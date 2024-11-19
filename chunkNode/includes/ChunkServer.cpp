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

    std::ofstream chunkFile(filename, std::ios::binary | std::ios::out);  
    if (!chunkFile.is_open()) {
            std::cerr << "Error opening output file for chunk " << filename << "." << std::endl;
        }

    chunkFile.write(data.data(), data.size());
        chunkFile.close();  

    chunkStorage[chunkID] = filename; 
    std::cout << "Stored chunk " << chunkID << " at " << filename << std::endl;
}
void ChunkServer::handleClientRequest(int clientSocket) {
    const size_t bufferSize = 1600; 
    char buffer[bufferSize];
    size_t totalBytesReceived = 0;
    std::vector<char> chunkData;

    ssize_t bytesRead;
    while ((bytesRead = recv(clientSocket, buffer, bufferSize, 0)) > 0) {
        totalBytesReceived += bytesRead;
        chunkData.insert(chunkData.end(), buffer, buffer + bytesRead);

        if (bytesRead < bufferSize) break;  
    }

    if (bytesRead < 0) {
        std::cerr << "Error reading from client socket." << std::endl;
        return;
    }

    std::string request(chunkData.begin(), chunkData.end());
  


    if (request.starts_with("STORE ")) {
        std::cout << "Received request: STORE" << std::endl;

        auto delimiterPos = request.find(" ");
        if (delimiterPos == std::string::npos) {
            std::cerr << "Invalid STORE command format." << std::endl;
            return;
        }

        auto chunkIDPos = request.find(" ", delimiterPos + 1);
        std::string chunkID = request.substr(delimiterPos + 1, chunkIDPos - delimiterPos - 1);
        std::string data = request.substr(chunkIDPos + 1);
        std::cout << "Received chunk: " << chunkID << ", Data Size: " << data.size() << " bytes" << std::endl;

        storeChunk(chunkID, data);  

    } else if (request.starts_with("RETRIEVE ")) {
        std::cout << "Received request: RETRIEVE" << std::endl;

        std::string chunkName = request.substr(9);  
        std::string chunkPath = "chunks/" + chunkName;

        std::ifstream chunkFile(chunkPath, std::ios::binary);
        if (chunkFile) {
            std::cout << "Found chunk: " << chunkName << std::endl;

            std::stringstream buffer;
            buffer << chunkFile.rdbuf();

            std::string chunkData = buffer.str();

            ssize_t bytesSent = send(clientSocket, chunkData.c_str(), chunkData.length(), 0);
            if (bytesSent < 0) {
                std::cerr << "Failed to send chunk data." << std::endl;
            } else {
                std::cout << "Sent chunk data for: " << chunkName << std::endl;
            }

        } else {
            std::cerr << "Chunk not found: " << chunkName << std::endl;

            std::string errorMessage = "ERROR: Chunk " + chunkName + " not found.";
            send(clientSocket, errorMessage.c_str(), errorMessage.length(), 0);
        }

    } else {
        std::string errorMsg = "ERROR: Invalid command\n";
        write(clientSocket, errorMsg.c_str(), errorMsg.size());
        std::cerr << "Invalid command received." << std::endl;
    }
}
