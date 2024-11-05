#include "MasterNode.hpp"
#include <iostream>

MasterNode::MasterNode() {}

void MasterNode::createFile(const std::string& filename) {
    fileMetadata[filename] = std::vector<ChunkLocation>();
    std::cout << "File " << filename << " created in metadata" << std::endl;
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
