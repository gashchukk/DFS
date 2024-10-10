#include "MasterNode.hpp"
#include <thread>
#include <chrono>
#include <random>
#include <cstring>
#include <arpa/inet.h>
#include <unistd.h>

MasterNode::MasterNode() {
    std::srand(std::time(nullptr));  // Seed for random chunk generation
}

// Register a new chunk server
void MasterNode::registerChunkServer(const std::string& serverId) {
    std::lock_guard<std::mutex> lock(mutex_);
    chunkServers_[serverId] = { "online", std::time(nullptr), {} };
    std::cout << "Chunk server " << serverId << " registered.\n";
}

// Update heartbeat for a server
void MasterNode::updateHeartbeat(const std::string& serverId) {
    std::lock_guard<std::mutex> lock(mutex_);
    if (chunkServers_.find(serverId) != chunkServers_.end()) {
        chunkServers_[serverId].lastHeartbeat = std::time(nullptr);
        std::cout << "Heartbeat updated for server " << serverId << ".\n";
    } else {
        std::cout << "Server " << serverId << " is not registered.\n";
    }
}

// Allocate a chunk for a file
void MasterNode::allocateChunk(const std::string& fileName) {
    std::lock_guard<std::mutex> lock(mutex_);
    int chunkId = std::rand() % 9000 + 1000;  // Generate random chunk ID
    metadata_[fileName] = chunkId;

    // Randomly assign a chunk server to store the chunk
    auto it = chunkServers_.begin();
    std::advance(it, std::rand() % chunkServers_.size());
    std::string serverId = it->first;
    chunkServers_[serverId].chunks.push_back(chunkId);

    std::cout << "Allocated chunk " << chunkId << " for file '" << fileName << "' on server " << serverId << ".\n";
}

// Get the chunk location for a file
std::pair<std::string, int> MasterNode::getChunkLocation(const std::string& fileName) {
    std::lock_guard<std::mutex> lock(mutex_);
    if (metadata_.find(fileName) != metadata_.end()) {
        int chunkId = metadata_[fileName];
        for (const auto& server : chunkServers_) {
            if (std::find(server.second.chunks.begin(), server.second.chunks.end(), chunkId) != server.second.chunks.end()) {
                return { server.first, chunkId };
            }
        }
    }
    return { "", -1 };
}

// Monitor chunk servers for heartbeats (run in a separate thread)
void MasterNode::monitorChunkServers() {
    while (true) {
        std::lock_guard<std::mutex> lock(mutex_);
        for (auto it = chunkServers_.begin(); it != chunkServers_.end();) {
            std::time_t currentTime = std::time(nullptr);
            if (currentTime - it->second.lastHeartbeat > 5) {  // 5-second timeout
                std::cout << "Server " << it->first << " is down (no heartbeat).\n";
                it = chunkServers_.erase(it);  // Remove the server from the list
            } else {
                ++it;
            }
        }
        std::this_thread::sleep_for(std::chrono::seconds(2));  // Sleep before next check
    }
}

// Handle incoming client (data node) connection
void MasterNode::handleClient(int clientSocket) {
    char buffer[1024];
    std::memset(buffer, 0, sizeof(buffer));

    // Read message from data node
    int bytesReceived = recv(clientSocket, buffer, sizeof(buffer), 0);
    if (bytesReceived > 0) {
        std::string message(buffer, bytesReceived);
        std::cout << "Received message from data node: " << message << std::endl;
        // Handle the message (heartbeat, chunk info, etc.)
        if (message == "HEARTBEAT") {
            updateHeartbeat("server1");  // Example server ID, can be parsed from message
        }
    }

    close(clientSocket);
}

// Start the master server to listen for incoming connections
void MasterNode::startMasterServer(int port) {
    int serverSocket = socket(AF_INET, SOCK_STREAM, 0);
    if (serverSocket == -1) {
        std::cerr << "Error creating socket.\n";
        return;
    }

    sockaddr_in serverAddr;
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(port);
    serverAddr.sin_addr.s_addr = INADDR_ANY;

    if (bind(serverSocket, (sockaddr*)&serverAddr, sizeof(serverAddr)) == -1) {
        std::cerr << "Error binding socket.\n";
        close(serverSocket);
        return;
    }

    if (listen(serverSocket, 5) == -1) {
        std::cerr << "Error listening on socket.\n";
        close(serverSocket);
        return;
    }

    std::cout << "Master server started, waiting for data nodes to connect...\n";

    while (true) {
        sockaddr_in clientAddr;
        socklen_t clientSize = sizeof(clientAddr);
        int clientSocket = accept(serverSocket, (sockaddr*)&clientAddr, &clientSize);

        if (clientSocket == -1) {
            std::cerr << "Error accepting client connection.\n";
            continue;
        }

        std::cout << "Data node connected.\n";

        // Handle the client connection in a new thread

        /*
        std::thread clientThread(&MasterNode::handleClient, this, clientSocket);
        clientThread.detach();
        */
    }

    close(serverSocket);
}
