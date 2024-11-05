#include "Client.h"
#include <iostream>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <cstring>
#include <sstream>
#include <vector>
#include <string>

Client::Client(const std::string& ip, int port) : masterIP(ip), masterPort(port) {}

std::vector<std::string> Client::getChunkServers(const std::string& filename) {
    std::vector<std::string> chunkServers;

    // Step 1: Connect to the Master Node
    int sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0) {
        std::cerr << "Error creating socket" << std::endl;
        return chunkServers;
    }

    struct sockaddr_in serverAddr;
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(masterPort);
    inet_pton(AF_INET, masterIP.c_str(), &serverAddr.sin_addr);

    if (connect(sock, (struct sockaddr*)&serverAddr, sizeof(serverAddr)) < 0) {
        std::cerr << "Error connecting to Master Node" << std::endl;
        close(sock);
        return chunkServers;
    }

    // Step 2: Send the "CREATE_FILE " command followed by the filename to the Master Node
    std::string request = "CREATE_FILE " + filename;
    send(sock, request.c_str(), request.length(), 0);

    // Step 3: Receive Chunk Server IP Address
    char buffer[1024];
    int bytesReceived = recv(sock, buffer, sizeof(buffer) - 1, 0);
    if (bytesReceived < 0) {
        std::cerr << "Error receiving data from Master Node" << std::endl;
        close(sock);
        return chunkServers;
    }
    buffer[bytesReceived] = '\0'; // Null-terminate the received data

    // Step 4: Parse the response to get Chunk Server IP
    std::string response(buffer);
    if (response.find("ChunkServerIP:") == 0) { // Expected format: "ChunkServerIP: <IP>"
        std::string chunkServerIP = response.substr(15); // Extract IP starting from index 14
        chunkServerIP.erase(chunkServerIP.find_last_not_of("\n\r") + 1); // Trim newline if any

        chunkServers.push_back(chunkServerIP); // Add the IP to chunkServers list

    } else {
        std::cerr << "Invalid response from Master Node: " << response << std::endl;
    }
   
    // Step 5: Close the Socket
    close(sock);
    //std::cout<<chunkServers[0];
    return chunkServers;
}


void Client::writeFile(const std::string& filename, const std::string& data) {
    std::vector<std::string> chunkServers = getChunkServers(filename);
    //std::cout<<chunkServers[0]<<std::endl;
    const size_t chunkSize = 64 ; // 64 KB
    size_t r = 1;
//    std::cout<<r;
    size_t n =0;

//    std::cout<<n<< "r:"<<r;
    size_t totalChunks = r + n;
    //std::cout << "Data size: " << data.size() << std::endl;

    for (size_t i = 0; i < totalChunks; ++i) {
        size_t start = i * chunkSize;
        size_t end = std::min(start + chunkSize, data.size());
        // Extract the chunk data
        std::string chunkData = data.substr(start, end - start);
        std::string chunkID = filename + "_" + std::to_string(i);
        if (!chunkServers.empty()) {
            int chunkServerSocket = socket(AF_INET, SOCK_STREAM, 0);
            if (chunkServerSocket < 0) {
                std::cerr << "Error creating socket for chunk server." << std::endl;
                return;
            }

            struct sockaddr_in serverAddr;
            serverAddr.sin_family = AF_INET;
            serverAddr.sin_port = htons(8081);
            inet_pton(AF_INET, chunkServers[0].c_str(), &serverAddr.sin_addr);

            if (connect(chunkServerSocket, (struct sockaddr*)&serverAddr, sizeof(serverAddr)) < 0) {
                std::cerr << "Connection to chunk server failed." << std::endl;
                close(chunkServerSocket);
                return;
            }

            std::string command = "STORE " + chunkID + " " + chunkData;
            std::cout<<command;
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
    auto chunkServers = getChunkServers(filename);

    if (chunkServers.empty()) {
        std::cerr << "No chunk servers available for retrieving file: " << filename << std::endl;
        return "";
    }

    std::string fileData;
    for (size_t i = 0; i < chunkServers.size(); ++i) {
        std::string chunkID = filename + "_" + std::to_string(i);
        int chunkServerSocket = socket(AF_INET, SOCK_STREAM, 0);

        if (chunkServerSocket < 0) {
            std::cerr << "Error creating socket for chunk server." << std::endl;
            return "";
        }

        struct sockaddr_in serverAddr;
        serverAddr.sin_family = AF_INET;
        serverAddr.sin_port = htons(8081);
        inet_pton(AF_INET, chunkServers[i].c_str(), &serverAddr.sin_addr);

        if (connect(chunkServerSocket, (struct sockaddr*)&serverAddr, sizeof(serverAddr)) < 0) {
            std::cerr << "Connection to chunk server failed for chunk " << chunkID << "." << std::endl;
            close(chunkServerSocket);
            return "";
        }

        std::string command = "RETRIEVE " + chunkID;
        send(chunkServerSocket, command.c_str(), command.size(), 0);

        char buffer[65536];
        int bytesRead = recv(chunkServerSocket, buffer, sizeof(buffer), 0);
        if (bytesRead > 0) {
            fileData.append(buffer, bytesRead);
        } else {
            std::cerr << "Failed to retrieve chunk " << chunkID << std::endl;
        }

        close(chunkServerSocket);
    }

    return fileData;
}
