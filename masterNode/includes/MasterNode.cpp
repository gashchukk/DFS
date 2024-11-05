#include "MasterNode.hpp"
#include <iostream>
#include <stdexcept>
#include <vector>
#include <random>


MasterNode::MasterNode() {
    // Initialize available chunk servers (example IPs)
    availableChunkServers = {"127.0.0.1:8081"};
}
std::string MasterNode::createFile(const std::string& filename) {
    if (fileMetadata.find(filename) != fileMetadata.end()) {
        throw std::runtime_error("File already exists");
    }
    std::string selectedServer = selectChunkServer();

    fileMetadata[filename] = {ChunkLocation{filename + "_chunk_0", {selectedServer}}};
    std::cout << "File " << filename << " sent request to write on " << selectedServer <<" and in was saved in metadata" <<std::endl;

    return selectedServer;
}

std::string MasterNode::selectChunkServer() {
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> distr(0, availableChunkServers.size() - 1);
    return availableChunkServers[distr(gen)];
}

void MasterNode::addChunk(const std::string& filename, const std::string& chunkID, const std::string& serverIP) {
    ChunkLocation chunkLocation = {chunkID, {serverIP}};
    fileMetadata[filename].push_back(chunkLocation);
    std::cout << "Added chunk " << chunkID << " for file " << filename << " on server " << serverIP << std::endl;
}

std::vector<ChunkLocation> MasterNode::getChunkLocations(const std::string& filename) {
    if (fileMetadata.find(filename) != fileMetadata.end()) {
        return fileMetadata[filename];
    } else {
        throw std::runtime_error("File not found");
    }
}
