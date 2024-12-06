#include "MasterNode.hpp"
#include <iostream>
#include <stdexcept>
#include <vector>
#include <random>


MasterNode::MasterNode() {
    availableChunkServers = {"127.0.0.1:8081"};
}


void MasterNode::createFile(const std::string& filename, const ChunkLocation& chunkLocation) {
    if (fileMetadata.find(filename) != fileMetadata.end()) {
        fileMetadata[filename].push_back(chunkLocation);
        std::cout << "Appended new chunk to existing file: " << filename << "\n";
    } else {
        fileMetadata[filename] = {chunkLocation};
        std::cout << "Created new file entry with first chunk: " << filename << "\n";
    }

    std::cout << "File " << filename << " now has " 
              << fileMetadata[filename].size() << " chunk(s).\n";
}

// void MasterNode::printFileMetadata() {
//     std::cout << "Current file metadata:" << std::endl;
//     for (const auto& fileEntry : fileMetadata) {
//         // Print the filename key
//         std::cout << "Filename: " << fileEntry.first << std::endl;

//         // Iterate through chunks associated with this file
//         for (const auto& chunk : fileEntry.second) {
//             std::cout << "  ChunkID: " << chunk.chunkID << ", ServerIP: " << chunk.serverIP << std::endl;
//         }
//     }
// }


std::vector<std::pair<std::string, std::string>> MasterNode::readFileRequest(const std::string& request) {
    std::vector<std::pair<std::string, std::string>> chunkInfo;

    size_t fileNameStart = request.find(':') + 1;
    std::string fileName = request.substr(fileNameStart);

    std::cout << "Received read request for file: " << fileName << std::endl;

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
