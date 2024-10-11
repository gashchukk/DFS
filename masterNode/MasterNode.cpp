#include "MasterNode.hpp"
#include <thread>
#include <chrono>
#include <random>
#include <cstring>
#include <arpa/inet.h>
#include <unistd.h>
#include <algorithm>

MasterNode::MasterNode()
{
    std::srand(std::time(nullptr));
}

void MasterNode::registerChunkServer(const std::string &serverId)
{
    std::lock_guard<std::mutex> lock(mutex_);
    chunkServers_[serverId] = {"online", std::time(nullptr), {}};
    std::cout << "Chunk server " << serverId << " registered.\n";
}

void MasterNode::updateHeartbeat(const std::string &serverId)
{
    std::lock_guard<std::mutex> lock(mutex_);
    if (chunkServers_.find(serverId) != chunkServers_.end())
    {
        chunkServers_[serverId].lastHeartbeat = std::time(nullptr);
        std::cout << "Heartbeat updated for server " << serverId << ".\n";
    }
    else
    {
        std::cout << "Server " << serverId << " is not registered.\n";
    }
}

void MasterNode::allocateChunk(const std::string &fileName)
{
    std::lock_guard<std::mutex> lock(mutex_);
    int chunkId = std::rand() % 9000 + 1000;
    metadata_[fileName] = chunkId;

    auto it = chunkServers_.begin();
    std::advance(it, std::rand() % chunkServers_.size());
    std::string serverId = it->first;
    chunkServers_[serverId].chunks.push_back(chunkId);

    std::cout << "Allocated chunk " << chunkId << " for file '" << fileName << "' on server " << serverId << ".\n";
}

std::pair<std::string, int> MasterNode::getChunkLocation(const std::string &fileName)
{
    std::lock_guard<std::mutex> lock(mutex_);
    if (metadata_.find(fileName) != metadata_.end())
    {
        int chunkId = metadata_[fileName];
        for (const auto &server : chunkServers_)
        {
            if (std::find(server.second.chunks.begin(), server.second.chunks.end(), chunkId) != server.second.chunks.end())
            {
                return {server.first, chunkId};
            }
        }
    }
    return {"", -1};
}

void MasterNode::monitorChunkServers()
{
    while (true)
    {
        std::lock_guard<std::mutex> lock(mutex_);
        for (auto it = chunkServers_.begin(); it != chunkServers_.end();)
        {
            std::time_t currentTime = std::time(nullptr);
            if (currentTime - it->second.lastHeartbeat > 5)
            {
                std::cout << "Server " << it->first << " is down (no heartbeat).\n";
                it = chunkServers_.erase(it);
            }
            else
            {
                ++it;
            }
        }
        std::this_thread::sleep_for(std::chrono::seconds(2));
    }
}

void MasterNode::handleClient(int clientSocket)
{
    char buffer[1024];
    std::memset(buffer, 0, sizeof(buffer));

    int bytesReceived = recv(clientSocket, buffer, sizeof(buffer), 0);
    if (bytesReceived > 0)
    {
        std::string message(buffer, bytesReceived);
        std::cout << "Received message from data node: " << message << std::endl;
        if (message == "HEARTBEAT")
        {
            updateHeartbeat("server1"); // Example server ID, can be parsed from message
        }
    }

    close(clientSocket);
}

void MasterNode::startMasterServer(int port)
{
    int serverSocket = socket(AF_INET, SOCK_STREAM, 0);
    if (serverSocket == -1)
    {
        std::cerr << "Error creating socket.\n";
        return;
    }

    sockaddr_in serverAddr;
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(port);
    serverAddr.sin_addr.s_addr = INADDR_ANY;

    if (bind(serverSocket, (sockaddr *)&serverAddr, sizeof(serverAddr)) == -1)
    {
        std::cerr << "Error binding socket.\n";
        close(serverSocket);
        return;
    }

    if (listen(serverSocket, 5) == -1)
    {
        std::cerr << "Error listening on socket.\n";
        close(serverSocket);
        return;
    }

    std::cout << "Master server started, waiting for data nodes to connect...\n";

    while (true)
    {
        sockaddr_in clientAddr;
        socklen_t clientSize = sizeof(clientAddr);
        int clientSocket = accept(serverSocket, (sockaddr *)&clientAddr, &clientSize);

        if (clientSocket == -1)
        {
            std::cerr << "Error accepting client connection.\n";
            continue;
        }

        std::cout << "Data node connected.\n";

        /*
        std::thread clientThread(&MasterNode::handleClient, this, clientSocket);
        clientThread.detach();
        */
    }

    close(serverSocket);
}
