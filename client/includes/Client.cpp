#include "Client.h"
#include <iostream>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <sys/stat.h>  // For stat() function and struct stat

#include <unistd.h>
#include <cstring>
#include <sstream>
#include <vector>
#include<filesystem>
#include <fstream>
#include <string>

#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include <sstream>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <algorithm>


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
#include <bitset>
#include <fstream>
#include <iostream>
#include <vector>
#include <string>
#include <filesystem>
#include <sstream>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
void Client::writeFile(const std::string& filepath) {
    std::filesystem::path path(filepath);
    std::string filename = path.filename().string();

    std::ifstream inputFile(filepath, std::ios::binary | std::ios::in);
    if (!inputFile.is_open()) {
        std::cerr << "Failed to open file: " << filepath << std::endl;
        return;
    }

    std::vector<char> fileData((std::istreambuf_iterator<char>(inputFile)),
                               std::istreambuf_iterator<char>());

    size_t fileSize = fileData.size();
    const size_t chunkSize = 1024; 
    size_t totalChunks = (fileSize / chunkSize) + (fileSize % chunkSize != 0 ? 1 : 0);
    std::cout << "File size: " << fileSize << " bytes, Total chunks: " << totalChunks << std::endl;

    std::vector<std::string> chunkServers = getChunkServers(filename);

    for (size_t i = 0; i < totalChunks; ++i) {
        size_t start = i * chunkSize;
        size_t end = std::min(start + chunkSize, fileSize);
        std::vector<char> chunkData(fileData.begin() + start, fileData.begin() + end);

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
            ssize_t bytesSent = send(chunkServerSocket, command.c_str(), command.size(), 0);
            if (bytesSent < 0) {
                std::cerr << "Failed to send STORE command." << std::endl;
                close(chunkServerSocket);
                return;
            }
             bytesSent = send(chunkServerSocket, chunkData.data(), chunkData.size(), 0);
            if (bytesSent < 0) {
                std::cerr << "Failed to send chunk " << chunkID << " data to chunk server." << std::endl;
                close(chunkServerSocket);
                return;
            } else {
                std::cout << "Sent chunk " << chunkID << ", bytes: " << bytesSent << std::endl;
            }


            close(chunkServerSocket);
            std::cout << "Sent chunk " << chunkID << " to chunk server " << chunkServerIP << ":" << chunkServerPort << "." << std::endl;
        } else {
            std::cerr << "No chunk servers available for storing the chunk." << std::endl;
            return;
        }
    }

    inputFile.close();
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

    char buffer[4096]; // Increased buffer size
    int bytesRead = recv(sock, buffer, sizeof(buffer), 0);
    if (bytesRead > 0) {
        std::string response(buffer, bytesRead);
        std::cout << "Received chunk locations: " << response << std::endl;

        // Parse chunk information (chunk name and chunk server IP)
        std::istringstream responseStream(response);
        std::vector<std::pair<std::string, std::string>> chunks;

        std::string chunkLine;
        while (std::getline(responseStream, chunkLine)) {
            size_t separatorPos = chunkLine.find(':');
            std::string chunkName = chunkLine.substr(0, separatorPos);
            std::string serverIP = chunkLine.substr(separatorPos + 1);
            chunks.push_back({chunkName, serverIP});
        }

        std::sort(chunks.begin(), chunks.end(), [](const auto& a, const auto& b) {
            return a.first < b.first;  
        });

        std::ofstream outputFile(fileName, std::ios::binary | std::ios::out);
        if (!outputFile.is_open()) {
            std::cerr << "Error opening output file" << std::endl;
            close(sock);
            return;
        }

        for (const auto& chunk : chunks) {
            const std::string& chunkName = chunk.first;
            const std::string& serverIP = chunk.second;

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
            std::cout<<"sent retrive\n";
            // Receive the chunk data and write it directly to the output file
            int totalBytesReceived = 0;
            while (true) {
                int chunkBytesRead = recv(chunkSock, buffer, sizeof(buffer), 0);
                if (chunkBytesRead > 0) {
                    totalBytesReceived += chunkBytesRead;
                    outputFile.write(buffer, chunkBytesRead);
                } else if (chunkBytesRead == 0) {
                    // EOF reached
                    break;
                } else {
                    std::cerr << "Error receiving chunk data for " << chunkName << std::endl;
                    break;
                }
            }

            std::cout << "Received " << totalBytesReceived << " bytes for chunk " << chunkName << std::endl;
            close(chunkSock);
        }

        outputFile.close();
        std::cout << "File " << fileName << " has been reconstructed successfully." << std::endl;
    } else {
        std::cerr << "Failed to receive chunk locations from Master Node" << std::endl;
    }

    close(sock);
}



void Client::listFiles() {

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

    std::string request = "LISTFILES";
    send(sock, request.c_str(), request.length(), 0);

    char buffer[1024];
    int bytesRead = recv(sock, buffer, sizeof(buffer), 0);
    if (bytesRead > 0) {
        std::string response(buffer, bytesRead);
        std::cout << "Received filenames from Master Node:" << std::endl;

    std::istringstream responseStream(response);
    std::string filename;
    std::cout<<" - chunks/\n";
    while (std::getline(responseStream, filename)) {
        std::cout << "      | - " << filename << std::endl;
    }


    close(sock);  
}}
