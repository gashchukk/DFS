#include "MasterNode.hpp"
#include <iostream>
#include <stdexcept>
#include <vector>
#include <random>
#include <netinet/in.h>
#include <unistd.h>
#include <arpa/inet.h>


MasterNode::MasterNode() {
    availableChunkServers = {};
}


void MasterNode::createFile(const std::string& filename, const ChunkLocation& chunkLocation) {
    if (fileMetadata.find(filename) != fileMetadata.end()) {
        fileMetadata[filename].push_back(chunkLocation);
        //std::cout << "Appended new chunk to existing file: " << filename << "\n";
    } else {
        fileMetadata[filename] = {chunkLocation};
       // std::cout << "Created new file entry with first chunk: " << filename << "\n";
    }

    std::cout << "File " << filename << " now has " 
              << fileMetadata[filename].size() << " chunk(s).\n";
}

// void MasterNode::printFileMetadata() {
//     std::cout << "SERVERS:" << std::endl;
//     for (const auto& element : availableChunkServers) {
//         std::cout << element << " ";
//     }

//     std::cout << "Current file metadata:" << std::endl;
//     for (const auto& fileEntry : fileMetadata) {
//         std::cout << "Filename: " << fileEntry.first << std::endl;

//         for (const auto& chunk : fileEntry.second) {
//             std::cout << "  ChunkID: " << chunk.chunkID << ", ServerIP: " << chunk.serverIP << std::endl;
//         }
//     }
// }


std::vector<std::pair<std::string, std::string>> MasterNode::readFileRequest(const std::string& request) {
    std::vector<std::pair<std::string, std::string>> chunkInfo;

    size_t fileNameStart = request.find(':') + 1;
    std::string fileName = request.substr(fileNameStart);


    //printFileMetadata();

    std::vector<ChunkLocation> chunkLocations;
        
    std::string trimmedName = [] (const std::string& str) -> std::string {
        auto start = str.find_first_not_of(" \t\n\r");
        auto end = str.find_last_not_of(" \t\n\r");
        return (start == std::string::npos || end == std::string::npos) ? "" : str.substr(start, end - start + 1);
    }(fileName);

    auto it = fileMetadata.find(trimmedName);
    if (it != fileMetadata.end()) {
        chunkLocations = it->second;
    } else {
        std::cerr << "File not found in metadata\n";
    }

    if (chunkLocations.empty()) {
        std::cerr << "No chunks found for the file: " << fileName << std::endl;
        return chunkInfo;
    }

    for (const auto& chunk : chunkLocations) {
        chunkInfo.push_back({chunk.chunkID, chunk.serverIP});
    }

    return chunkInfo;
}


std::string MasterNode::selectChunkServer() {
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> distr(0, availableChunkServers.size() - 1);
    return availableChunkServers[distr(gen)];
}

std::vector<std::string> MasterNode::selectTwoDifferentServers(std::string& currentServer) {

    std::vector<std::string> validServers;
    for (const auto& server : availableChunkServers) {
        if (server != currentServer) {
            validServers.push_back(server);
        }
        if(validServers.size() == 2){
            break;
        }
    }
    return validServers;
}
void MasterNode::sendSelectedServersToCurrentServer(std::string& currentServer, std::string& chunkName) {
    std::vector<std::string> selectedServers = selectTwoDifferentServers(currentServer);
    std::string targetServer;
    if (selectedServers.size() == 2) {
        targetServer = selectedServers[0] + "," + selectedServers[1];
    } else if (selectedServers.size() == 1) {
        targetServer = selectedServers[0];
    } else {
        std::cerr << "Not enough available servers to send to." << std::endl;
        return;
    }

    int sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0) {
        std::cerr << "Socket creation error" << std::endl;
        return;
    }

    struct sockaddr_in serverAddr;
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_addr.s_addr = INADDR_ANY;
    serverAddr.sin_port = htons(std::stoi(currentServer.substr(currentServer.find(':') + 1))); // Extract port from currentServer

    if (inet_pton(AF_INET, currentServer.substr(0, currentServer.find(':')).c_str(), &serverAddr.sin_addr) <= 0) {
        std::cerr << "Invalid address/ Address not supported" << std::endl;
        close(sock);
        return;
    }

    if (connect(sock, (struct sockaddr*)&serverAddr, sizeof(serverAddr)) < 0) {
        std::cerr << "Connection Failed" << std::endl;
        close(sock);
        return;
    }

    std::string serversMessage = "SERVERS " + chunkName + " " + targetServer;
    uint32_t length = serversMessage.size();

    send(sock, &length, sizeof(length), 0);

    size_t bytesSend = send(sock, serversMessage.c_str(), serversMessage.size(), 0);
    std::cout << "Sent message: " << length << " bytes\n" << serversMessage <<"\n Bytes send:"+std::to_string(bytesSend) <<std::endl;

    close(sock);
}

