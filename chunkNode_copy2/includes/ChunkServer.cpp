#include <iostream>
#include <fstream>
#include <sys/socket.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <cstring>
#include <sys/stat.h>
#include <sys/types.h>
#include <filesystem>
#include <regex>
#include <vector>
#include <algorithm>
#include <sstream>
#include "ChunkServer.h" 

 ChunkServer::ChunkServer(int port) : serverPort(port) {}

int ChunkServer::getServerPort() const {
    return serverPort;
}

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

    // Notify the master server about the new chunk server
    notifyMaster();

    while (true) {
        int clientSocket = accept(serverSocket, nullptr, nullptr);
        if (clientSocket >= 0) {
            handleClientRequest(clientSocket);  // Assuming you have this function to handle requests
            shutdown(clientSocket, SHUT_WR); 
            close(clientSocket);
        }
    }
}

void ChunkServer::notifyMaster() {
    // Create a client socket to notify the master server
    int clientSocket = socket(AF_INET, SOCK_STREAM, 0);
    if (clientSocket < 0) {
        std::cerr << "Error creating client socket to notify master" << std::endl;
        return;
    }

    struct sockaddr_in masterAddr;
    masterAddr.sin_family = AF_INET;
    masterAddr.sin_port = htons(8080);  // Master server port
    masterAddr.sin_addr.s_addr = inet_addr("127.0.0.1");

    if (connect(clientSocket, (struct sockaddr*)&masterAddr, sizeof(masterAddr)) < 0) {
        std::cerr << "Error connecting to master server" << std::endl;
        close(clientSocket);
        return;
    }

    std::string notificationMessage = "NEW_SERVER 127.0.0.1:" + std::to_string(serverPort);
    ssize_t bytesSent = send(clientSocket, notificationMessage.c_str(), notificationMessage.length(), 0);
    if (bytesSent < 0) {
        std::cerr << "Error sending notification to master server" << std::endl;
    } else {
        std::cout << "Notified master server of new chunk server at 127.0.0.1:" << serverPort << std::endl;
    }

    close(clientSocket);
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


    uint32_t length;
    recv(clientSocket, &length, sizeof(length), 0);  
    std::vector<char> headerBuff(length);
    ssize_t header_read = recv(clientSocket, headerBuff.data(), length, 0);

    if (header_read <= 0) {
        if (header_read == 0) {
            std::cerr << "Client closed connection." << std::endl;
        } else {
            std::cerr << "Error reading header from client socket." << std::endl;
        }
        return;
    }

    std::string header(headerBuff.begin(), headerBuff.begin() + header_read);

    
    if (header.starts_with("STORE ")) {
        std::cout << "Received request: STORE" << std::endl;

        auto delimiterPos = header.find(" ");  
        if (delimiterPos == std::string::npos) {
            std::cout << header << std::endl;
            std::cerr << "Invalid STORE command format." << std::endl;
            return;
        }

        std::string chunkID = header.substr(6, delimiterPos - 6);  // Extract chunkID
        std::cout << "Chunk ID: " << chunkID << std::endl;

        std::vector<char> chunkData;
        const size_t bufferSize = 1024;
        char buffer[bufferSize];

        size_t totalBytesReceived = 0;
        ssize_t bytesRead;
        while (true) {
            bytesRead = recv(clientSocket, buffer, bufferSize, 0);
            if (bytesRead <= 0) {
                if (bytesRead == 0) {
                    std::cerr << "Client closed connection." << std::endl;
                } else {
                    std::cerr << "Error reading chunk data from client." << std::endl;
                }
                break;
            }

            totalBytesReceived += bytesRead;
            chunkData.insert(chunkData.end(), buffer, buffer + bytesRead);

            if (bytesRead < bufferSize) {
                break;  
            }
        }

        std::string data(chunkData.begin(), chunkData.end());
        std::cout << "Received chunk data of size: " << data.size() << " bytes." << std::endl;
        storeChunk(chunkID, data);  

    } else if (header.starts_with("RETRIEVE ")) {
        std::cout << "Received request: RETRIEVE" << std::endl;

        std::string chunkName = header.substr(9);  
        std::cout<<header<<"\n"<<chunkName<<"\n";
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
