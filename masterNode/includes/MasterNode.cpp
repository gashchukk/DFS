#include "MasterNode.hpp"
#include <iostream>
#include <stdexcept>
#include <vector>
#include <random>


MasterNode::MasterNode() {
    // Initialize available chunk servers (example IPs)
    availableChunkServers = {"127.0.0.1:8081"};
}
void MasterNode::createFile(const std::string& filename, const ChunkLocation& chunkLocation) {
    // Check if the file already exists in the metadata
    if (fileMetadata.find(filename) != fileMetadata.end()) {
        // File exists, append the new chunk location
        fileMetadata[filename].push_back(chunkLocation);
        std::cout << "Appended new chunk to existing file: " << filename << "\n";
    } else {
        // File does not exist, create a new entry
        fileMetadata[filename] = {chunkLocation};
        std::cout << "Created new file entry with first chunk: " << filename << "\n";
    }

    // Debug log
    std::cout << "File " << filename << " now has " 
              << fileMetadata[filename].size() << " chunk(s).\n";
}



std::string MasterNode::selectChunkServer() {
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> distr(0, availableChunkServers.size() - 1);
    return availableChunkServers[distr(gen)];
}
