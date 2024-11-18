#include "Client.h"
#include <iostream>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <cstring>
#include <sstream>
#include <vector>
#include<filesystem>
#include <fstream>
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
    return chunkServers;
}

void Client::writeFile(const std::string& filepath) {

    std::filesystem::path path(filepath);
     std::string filename= path.filename().string();
    std::ifstream inputFile(filepath, std::ios::binary);
    if (!inputFile) {
        std::cerr << "Failed to open file: " << filename << std::endl;
        return;
    }
    std::vector<char> data((std::istreambuf_iterator<char>(inputFile)), std::istreambuf_iterator<char>());
    inputFile.close(); 

    std::vector<std::string> chunkServers = getChunkServers(filename);
    const size_t chunkSize = 256; 

    size_t totalChunks = (data.size() / chunkSize) + (data.size() % chunkSize != 0 ? 1 : 0);

    for (size_t i = 0; i < totalChunks; ++i) {
        size_t start = i * chunkSize;
        size_t end = std::min(start + chunkSize, data.size());
        std::vector<char> chunkData(data.begin() + start, data.begin() + end);
        std::string chunkID = filename + "_" + std::to_string(i);

        if (!chunkServers.empty()) {
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
            std::string command = "STORE " + chunkID + " ";

            send(chunkServerSocket, command.c_str(), command.size(), 0);

            ssize_t bytesSent = send(chunkServerSocket, chunkData.data(), chunkData.size(), 0);
            if (bytesSent < 0) {
                std::cerr << "Failed to send chunk " << chunkID << " to chunk server." << std::endl;
                close(chunkServerSocket);
                return;
            }

            close(chunkServerSocket); 

            std::cout << "Sent chunk " << chunkID << " to chunk server " << chunkServerIP << ":" << chunkServerPort << "." << std::endl;
        } else {
            std::cerr << "No chunk servers available for storing the chunk." << std::endl;
            return;
        }
    }
}

void Client::readfile(const std::string& fileName) {
    int sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0) {
        std::cerr << "Error creating socket" << std::endl;
        return;
    }

    struct sockaddr_in serverAddr;
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(masterPort);
    inet_pton(AF_INET, masterIP.c_str(), &serverAddr.sin_addr);

    if (connect(sock, (struct sockaddr*)&serverAddr, sizeof(serverAddr)) < 0) {
        std::cerr << "Error connecting to Master Node" << std::endl;
        close(sock);
        return;
    }

    std::string request = "READFILE: " + fileName;
    send(sock, request.c_str(), request.length(), 0);

    char buffer[1024];
    int bytesRead = recv(sock, buffer, sizeof(buffer), 0);
    if (bytesRead > 0) {
        std::string response(buffer, bytesRead);
        std::cout << "Received chunk locations: " << response << std::endl;

        std::ofstream outFile(fileName, std::ios::binary | std::ios::out);
        if (!outFile) {
            std::cerr << "Error opening output file" << std::endl;
            close(sock);
            return;
        }

        std::istringstream responseStream(response);
        std::string chunkLine;
        while (std::getline(responseStream, chunkLine)) {
            size_t separatorPos = chunkLine.find(':');
            std::string chunkName = chunkLine.substr(0, separatorPos);
            std::string serverIP = chunkLine.substr(separatorPos + 1);
            std::cout << "Requesting chunk: " << chunkName << " from server: " << serverIP << std::endl;

            int chunkSock = socket(AF_INET, SOCK_STREAM, 0);
            if (chunkSock < 0) {
                std::cerr << "Error creating socket for chunk server" << std::endl;
                continue; 
            }

            struct sockaddr_in chunkServerAddr;
            chunkServerAddr.sin_family = AF_INET;
            chunkServerAddr.sin_port = htons(8081);  
            inet_pton(AF_INET, serverIP.c_str(), &chunkServerAddr.sin_addr);

            if (connect(chunkSock, (struct sockaddr*)&chunkServerAddr, sizeof(chunkServerAddr)) < 0) {
                std::cerr << "Error connecting to Chunk Server at " << serverIP << std::endl;
                close(chunkSock);
                continue; 
            }

            std::string chunkRequest = "RETRIEVE " + chunkName;
            send(chunkSock, chunkRequest.c_str(), chunkRequest.length(), 0);

            char chunkBuffer[1024];
            int chunkBytesRead = recv(chunkSock, chunkBuffer, sizeof(chunkBuffer), 0);
            if (chunkBytesRead > 0) {
                std::string chunkData(chunkBuffer, chunkBytesRead);

                outFile.write(chunkData.c_str(), chunkData.length());
            } else {
                std::cerr << "Failed to receive chunk data for " << chunkName << std::endl;
            }

            close(chunkSock); 
        }

        outFile.close(); 
        std::cout << "File " << fileName << " has been written successfully." << std::endl;
    } else {
        std::cerr << "Failed to receive chunk locations from Master Node" << std::endl;
    }

    close(sock); 
}
