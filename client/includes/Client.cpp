#include "Client.h"
#include <iostream>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <cstring>

Client::Client(const std::string& ip, int port) : masterIP(ip), masterPort(port) {}

std::vector<std::pair<std::string, std::string>> Client::getChunkServers(const std::string& filename) {
    // Connect to Master Node for metadata
    int sock = socket(AF_INET, SOCK_STREAM, 0);
    struct sockaddr_in serverAddr;
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(masterPort);
    inet_pton(AF_INET, masterIP.c_str(), &serverAddr.sin_addr);

    connect(sock, (struct sockaddr*)&serverAddr, sizeof(serverAddr));
    send(sock, filename.c_str(), filename.length(), 0);

    // Receive chunk server locations from Master Node
    // (Parsing logic based on protocol implementation)
    // Close socket
    close(sock);

    return {}; // Placeholder: return chunk server info
}

void Client::writeFile(const std::string& filename, const std::string& data) {
    // Assuming getChunkServers is a method that returns a vector of IP addresses of the chunk servers
    auto chunkServers = getChunkServers(filename);

    const size_t chunkSize = 64 * 1024; // 64 KB
    size_t totalChunks = (data.size() / chunkSize) + (data.size() % chunkSize != 0 ? 1 : 0);

    for (size_t i = 0; i < totalChunks; ++i) {
        // Calculate the start and end indices for the current chunk
        size_t start = i * chunkSize;
        size_t end = std::min(start + chunkSize, data.size());

        // Extract the chunk data
        std::string chunkData = data.substr(start, end - start);
        std::string chunkID = filename + "_" + std::to_string(i); // Create a unique chunkID

        // Send the chunk to the appropriate chunk server
        // For simplicity, let's send to the first server; in practice, you should distribute across servers
        if (!chunkServers.empty()) {
            int chunkServerSocket = socket(AF_INET, SOCK_STREAM, 0);
            if (chunkServerSocket < 0) {
                std::cerr << "Error creating socket for chunk server." << std::endl;
                return;
            }

            struct sockaddr_in serverAddr;
            serverAddr.sin_family = AF_INET;
            serverAddr.sin_port = htons(8081); // Assuming each Chunk Server listens on port 5001
            inet_pton(AF_INET, chunkServers[0].c_str(), &serverAddr.sin_addr); // Convert IP to binary form

            if (connect(chunkServerSocket, (struct sockaddr*)&serverAddr, sizeof(serverAddr)) < 0) {
                std::cerr << "Connection to chunk server failed." << std::endl;
                close(chunkServerSocket);
                return;
            }

            // Prepare STORE command
            std::string command = "STORE " + chunkID + " " + chunkData;
            send(chunkServerSocket, command.c_str(), command.size(), 0);
            close(chunkServerSocket);
            std::cout << "Sent chunk " << chunkID << " to chunk server." << std::endl;
        } else {
            std::cerr << "No chunk servers available for storing the chunk." << std::endl;
            return;
        }
    }
}
std::string Client::readFile(const std::string& filename) {
    auto chunkServers = getChunkServers(filename); // Retrieve chunk server info from Master Node

    if (chunkServers.empty()) {
        std::cerr << "No chunk servers available for retrieving file: " << filename << std::endl;
        return "";
    }

    std::string fileData;
    for (size_t i = 0; i < chunkServers.size(); ++i) {
        // Retrieve each chunk from the respective chunk server
        std::string chunkID = filename + "_" + std::to_string(i);
        int chunkServerSocket = socket(AF_INET, SOCK_STREAM, 0);

        if (chunkServerSocket < 0) {
            std::cerr << "Error creating socket for chunk server." << std::endl;
            return "";
        }

        struct sockaddr_in serverAddr;
        serverAddr.sin_family = AF_INET;
        serverAddr.sin_port = htons(5001); // Assuming each Chunk Server listens on port 5001
        inet_pton(AF_INET, chunkServers[i].c_str(), &serverAddr.sin_addr);

        if (connect(chunkServerSocket, (struct sockaddr*)&serverAddr, sizeof(serverAddr)) < 0) {
            std::cerr << "Connection to chunk server failed for chunk " << chunkID << "." << std::endl;
            close(chunkServerSocket);
            return "";
        }

        // Send RETRIEVE command to the chunk server
        std::string command = "RETRIEVE " + chunkID;
        send(chunkServerSocket, command.c_str(), command.size(), 0);

        // Read the chunk data from the server
        char buffer[65536]; // 64KB buffer for chunk data
        int bytesRead = recv(chunkServerSocket, buffer, sizeof(buffer), 0);
        if (bytesRead > 0) {
            fileData.append(buffer, bytesRead); // Append received chunk to file data
        } else {
            std::cerr << "Failed to retrieve chunk " << chunkID << std::endl;
        }

        close(chunkServerSocket);
    }

    return fileData;
}
