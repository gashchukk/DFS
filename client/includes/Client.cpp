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

     std::string request = "GET_CHUNK_LOCATION " + filename;
     send(sock, request.c_str(), request.length(), 0);

    char buffer[1024];
    std::string response;
    int bytesReceived;
    while ((bytesReceived = recv(sock, buffer, sizeof(buffer) - 1, 0)) > 0) {
        buffer[bytesReceived] = '\0'; 
        response += buffer;
        if (response.find("\n") != std::string::npos) { // End-of-message marker
            break;
        }
    }

    if (bytesReceived < 0) {
        std::cerr << "Error receiving data from Master Node" << std::endl;
        close(sock);
        return chunkServers;
    }


    if (response.rfind("ChunkServerIP:", 0) == 0) { 
        std::string chunkServerIP = response.substr(15); 

        chunkServerIP.erase(chunkServerIP.find_last_not_of(" \n\r\t") + 1); //trim from end
        chunkServerIP.erase(0, chunkServerIP.find_first_not_of(" \n\r\t")); //trim fron start


        chunkServers.push_back(chunkServerIP);
        std::cout << "Stored ChunkServerIP: [" << chunkServers[0] << "]" << std::endl;
    } else {
        std::cerr << "Invalid response from Master Node: " << response << std::endl;
    }

    close(sock);
    std::cout<<chunkServers[0];
        return chunkServers;
}

void Client::writeFile(const std::string& filename, const std::string& data) {
    std::vector<std::string> chunkServers = getChunkServers(filename);
    const size_t chunkSize = 64; // 64 bytes for demonstration

    size_t totalChunks = (data.size() / chunkSize) + (data.size() % chunkSize != 0 ? 1 : 0);

    for (size_t i = 0; i < totalChunks; ++i) {
        size_t start = i * chunkSize;
        size_t end = std::min(start + chunkSize, data.size());
        std::string chunkData = data.substr(start, end - start);
        std::string chunkID = filename + "_" + std::to_string(i) + ".txt";

        if (!chunkServers.empty()) {
            // Parse IP and port
            std::string chunkServerIPPort = chunkServers[0]; 
            size_t colonPos = chunkServerIPPort.find(':');
            if (colonPos == std::string::npos) {
                std::cerr << "Invalid IP:Port format: " << chunkServerIPPort << std::endl;
                return;
            }

            std::string chunkServerIP = chunkServerIPPort.substr(0, colonPos);
            int chunkServerPort = std::stoi(chunkServerIPPort.substr(colonPos + 1));

            int chunkServerSocket = socket(AF_INET, SOCK_STREAM, 0);
            if (chunkServerSocket < 0) {
                std::cerr << "Error creating socket for chunk server." << std::endl;
                return;
            }

            struct sockaddr_in serverAddr;
            serverAddr.sin_family = AF_INET;
            serverAddr.sin_port = htons(chunkServerPort);
            if (inet_pton(AF_INET, chunkServerIP.c_str(), &serverAddr.sin_addr) <= 0) {
                std::cerr << "Invalid IP address format: " << chunkServerIP << std::endl;
                close(chunkServerSocket);
                return;
            }

            if (connect(chunkServerSocket, (struct sockaddr*)&serverAddr, sizeof(serverAddr)) < 0) {
                std::cerr << "Connection to chunk server failed: " << chunkServerIP << ":" << chunkServerPort << std::endl;
                close(chunkServerSocket);
                return;
            }

            std::string command = "STORE " + chunkID + " " + chunkData;
            send(chunkServerSocket, command.c_str(), command.size(), 0);
            close(chunkServerSocket);

            std::cout << "Sent chunk " << chunkID << " to chunk server " << chunkServerIP << ":" << chunkServerPort << "." << std::endl;
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
