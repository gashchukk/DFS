#include "MasterNode.hpp"
#include <iostream>
#include <stdexcept>
#include <vector>
#include <random>
#include <netinet/in.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <unordered_map>
#include <string>
#include <algorithm>
#include<chrono>
#include <unistd.h> // For sleep()

MasterNode::MasterNode() {
    availableChunkServers = {};
}


void MasterNode::createFile(const std::string& filename, const ChunkLocation& chunkLocation) {
    if (fileMetadata.find(filename) != fileMetadata.end()) {
        fileMetadata[filename].push_back(chunkLocation);
    } else {
        fileMetadata[filename] = {chunkLocation};
    }

    std::cout << "File " << filename << " now has " 
              << fileMetadata[filename].size() << " chunk(s).\n";
}

void MasterNode::printFileMetadata() {
    std::cout << "SERVERS:" << std::endl;
    for (const auto& element : availableChunkServers) {
        std::cout << element << " ";
    }

    std::cout << "Current file metadata:" << std::endl;
    for (const auto& fileEntry : fileMetadata) {
        std::cout << "Filename: " << fileEntry.first << std::endl;

        for (const auto& chunk : fileEntry.second) {
            std::cout << "  ChunkID: " << chunk.chunkID << ", ServerIP: " << chunk.serverIP << std::endl;
        }
    }
}

std::vector<std::pair<std::string, std::string>> MasterNode::readFileRequest(const std::string& request) {
    std::vector<std::pair<std::string, std::string>> chunkInfo;
    std::string baseFileName = request.substr(request.find(':') + 1);

    std::string trimmedName = [] (const std::string& str) -> std::string {
        auto start = str.find_first_not_of(" \t\n\r");
        auto end = str.find_last_not_of(" \t\n\r");
        return (start == std::string::npos || end == std::string::npos) ? "" : str.substr(start, end - start + 1);
    }(baseFileName);

    std::vector<ChunkLocation> chunkLocations;
   
        auto it = fileMetadata.find(trimmedName);
        if (it != fileMetadata.end()) {
            chunkLocations = it->second;
        } else {
            std::cerr << " File not found in metadata\n";
        }
    

    if (chunkLocations.empty()) {
        std::cerr << "File not found in any copy\n";
        return chunkInfo;
    }
    size_t i =0;
    std::string fileNameWithAttempt = trimmedName + "_" + std::to_string(i);
    while (true){
    for (const auto& chunk : chunkLocations) {
        if (chunk.chunkID.find(fileNameWithAttempt) != std::string::npos) {
            chunkInfo.push_back({chunk.chunkID, chunk.serverIP});
        }else{

        }
    }
    }

    return chunkInfo;
}


std::string MasterNode::selectChunkServer() {
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> distr(0, availableChunkServers.size() - 1);
    return availableChunkServers[distr(gen)];
}

std::vector<std::string> MasterNode::selectTwoDifferentServers(std::string& currentServer, std::string chunkName) {
    std::vector<std::string> validServers = availableChunkServers;
    size_t firstUnderscorePos = chunkName.find('_');
    std::string copyNumber;
    if (firstUnderscorePos != std::string::npos) {
        copyNumber = chunkName.substr(firstUnderscorePos + 1);
        chunkName = chunkName.substr(0, firstUnderscorePos);
    }

    size_t lastUnderscorePos2 = chunkName.find_last_of('_');
    std::string fileName = chunkName.substr(0, lastUnderscorePos2);

    auto it = fileMetadata.find(fileName);
    if (it != fileMetadata.end()) {
        std::vector<ChunkLocation>& locations = it->second;
        for (const auto& location : locations) {
            if (location.chunkID.find("_" + copyNumber)) {
                validServers.erase(std::remove(validServers.begin(), validServers.end(), location.serverIP), validServers.end());
            }
        }
    }

    validServers.erase(std::remove(validServers.begin(), validServers.end(), currentServer), validServers.end());

    if (validServers.size() > 2) {
        std::random_device rd;
        std::mt19937 g(rd());
        std::shuffle(validServers.begin(), validServers.end(), g);
        validServers.resize(2);
    }

    return validServers;
}






void MasterNode::sendSelectedServersToCurrentServer(std::string& currentServer, std::string& chunkName) {
    std::vector<std::string> selectedServers = selectTwoDifferentServers(currentServer, chunkName);
    std::string targetServer;
    /*if (selectedServers.size() == 2) {
        targetServer = selectedServers[0] + "," + selectedServers[1];
    } else if (selectedServers.size() == 1) {*/
        targetServer = selectedServers[0];
    /*} else {
        std::cerr << "Not enough available servers to send to." << std::endl;
        return;
    }*/

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

