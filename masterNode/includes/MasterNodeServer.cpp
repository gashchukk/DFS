// MasterNodeServer.cpp
#include "MasterNodeServer.hpp"

#include <iostream>
#include <netinet/in.h>
#include <unistd.h>
#include <cstring>
#include <iostream>
#include <string>
#include <thread>

MasterNodeServer::MasterNodeServer(int port) : serverPort(port) {}

std::unordered_map<std::string, std::chrono::time_point<std::chrono::steady_clock>> activeChunkServers;

void MasterNodeServer::start() {
    int serverSocket = socket(AF_INET, SOCK_STREAM, 0);
    if (serverSocket < 0) {
        std::cerr << "Error creating socket" << std::endl;
        return;
    }

    struct sockaddr_in serverAddr;
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_addr.s_addr = INADDR_ANY;
    serverAddr.sin_port = htons(serverPort);

    if (bind(serverSocket, (struct sockaddr*)&serverAddr, sizeof(serverAddr)) < 0) {
        std::cerr << "Binding failed" << std::endl;
        return;
    }

    listen(serverSocket, 5);
    std::cout << "Master Node listening on port " << serverPort << std::endl;

    while (true) {
        int clientSocket = accept(serverSocket, nullptr, nullptr);
        if (clientSocket >= 0) {
            std::cout << "Client or Chunk Server connected" << std::endl;
            handleConnection(clientSocket);
            close(clientSocket);
        }
    }
}



void MasterNodeServer::handleConnection(int clientSocket) {
    char buffer[1024];
    int bytesReceived = recv(clientSocket, buffer, sizeof(buffer), 0);
    if (bytesReceived <= 0) {
        std::cerr << "Error receiving data from client" << std::endl;
        return;
    }

    std::string request(buffer, bytesReceived);
    std::string response;
    std::string chunkServerIP;
    if(request.starts_with("GET_CHUNK_LOCATION")) {
        try{
            chunkServerIP = masterNode.selectChunkServer();
            response = "ChunkServerIP: " + chunkServerIP + "\n";
            send(clientSocket, response.c_str(), response.size(), 0);
        }catch(const std::exception& e) {
            response = std::string("Error: ") + e.what() + "\n";
        }
    }

   if (request.starts_with("HEARTBEAT:")) {
    size_t ipStart = request.find(':') + 1; 
    size_t ipEnd = request.find(':', ipStart); 
    std::string masterIP = request.substr(ipStart, ipEnd - ipStart);

    size_t portStart = ipEnd + 1;
    size_t portEnd = request.find(':', portStart); 
    std::string masterPort = request.substr(portStart, portEnd - portStart);

    std::string serverIPWithPort = masterIP + ":" + masterPort;

    std::string chunkName = request.substr(portEnd + 1);
    chunkName.erase(std::find_if(chunkName.rbegin(), chunkName.rend(), [](unsigned char ch) {
    return !std::isspace(ch);
}).base(), chunkName.end());  
    size_t underscorePos = chunkName.rfind('_');
    std::string filename = chunkName.substr(0, underscorePos);

    std::cout << "heartbeat: " << serverIPWithPort
              << " for chunk " << chunkName << " (filename: " << filename << ")" << std::endl;

    ChunkLocation chunk(chunkName, serverIPWithPort);
    masterNode.createFile(filename, chunk);
    masterNode.sendSelectedServersToCurrentServer(serverIPWithPort, chunkName);

}
   else if(request.starts_with("READFILE:")){
        std::vector<std::pair<std::string, std::string>> chunkLocations = masterNode.readFileRequest(request);

        std::string response;
    for (const auto& chunk : chunkLocations) {
        response += chunk.first + ":" + chunk.second + "\n";  
    }

    send(clientSocket, response.c_str(), response.size(), 0);
    //std::cout << "Sent chunk locations to client: " << response << std::endl;
    
    } else if (request.starts_with("LISTFILES")){
        std::string allFiles;
        for (const auto& pair : masterNode.fileMetadata) {
            allFiles += pair.first + "\n";  
        }
        send(clientSocket, allFiles.c_str(), allFiles.length(), 0);
   
    } else if(request.starts_with("NEW_SERVER")){
        size_t ipStart = request.find(' ') + 1;
        std::string chunkServerIP = request.substr(ipStart);
        std::cout << "New chunkServer connected at: " << chunkServerIP << std::endl;
        masterNode.availableChunkServers.push_back(chunkServerIP);
    }

}
