#include "MasterNode.hpp"
#include <iostream>
#include <stdexcept>
#include <vector>
#include <random>


MasterNode::MasterNode() {
    // Initialize available chunk servers (example IPs)
    availableChunkServers = {"127.0.0.1:8081"};
}
void MasterNode::createFile(const std::string& filename, std::string& selectedServer) {
    if (fileMetadata.find(filename) != fileMetadata.end()) {
        throw std::runtime_error("File already exists");
    }

    fileMetadata[filename] = {ChunkLocation{filename + "_chunk_0", {selectedServer}}};
    std::cout << "File " << filename << " sent request to write on " << selectedServer <<" and was saved in metadata" <<std::endl;

}

std::string MasterNode::selectChunkServer() {
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> distr(0, availableChunkServers.size() - 1);
    return availableChunkServers[distr(gen)];
}
