#include <iostream>
#include <fstream>
#include <sys/socket.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <cstring>
#include <sys/stat.h>
#include <sys/types.h>
#include <filesystem>
#include <regex>
#include <vector>
#include <algorithm>
#include <sstream>
#include "ChunkServer.h" 
#include <algorithm> // for std::find_if
#include <cctype>    // for std::isspace
#include<thread>
#include <iostream>
#include <fstream>
#include <sstream>
#include <filesystem>
#include <cstring>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

std::mutex mutex;
ChunkServer::ChunkServer(int port) : serverPort(port) {}

int ChunkServer::getServerPort() const {
    return serverPort;
}

void ChunkServer::start() {
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
    std::cout << "Chunk Server listening on port " << serverPort << std::endl;

    notifyMaster();

    while (true) {
        int clientSocket = accept(serverSocket, nullptr, nullptr);
        if (clientSocket >= 0) {
            std::cout << "Client connected" << std::endl;

            std::thread clientThread(&ChunkServer::handleClientRequest, this, clientSocket);
            clientThread.detach();
        }
    }
}

void ChunkServer::notifyMaster() {
    int clientSocket = socket(AF_INET, SOCK_STREAM, 0);
    if (clientSocket < 0) {
        std::cerr << "Error creating client socket to notify master" << std::endl;
        return;
    }

    struct sockaddr_in masterAddr;
    masterAddr.sin_family = AF_INET;
    masterAddr.sin_port = htons(8080);  
    masterAddr.sin_addr.s_addr = inet_addr("127.0.0.1");

    if (connect(clientSocket, (struct sockaddr*)&masterAddr, sizeof(masterAddr)) < 0) {
        std::cerr << "Error connecting to master server" << std::endl;
        close(clientSocket);
        return;
    }

    std::string notificationMessage = "NEW_SERVER 127.0.0.1:" + std::to_string(serverPort);
    ssize_t bytesSent = send(clientSocket, notificationMessage.c_str(), notificationMessage.length(), 0);
    if (bytesSent < 0) {
        std::cerr << "Error sending notification to master server" << std::endl;
    } else {
        std::cout << "Notified master server of new chunk server at 127.0.0.1:" << serverPort << std::endl;
    }

    close(clientSocket);
}


void ChunkServer::storeChunk( std::string& chunkID, const std::string& data) {
    chunkID.erase(chunkID.find_last_not_of(" \t\n\r\f\v") + 1);
    std::string filename = "chunks/" + chunkID; 

    std::ofstream chunkFile(filename, std::ios::binary | std::ios::out);  
    if (!chunkFile.is_open()) {
            std::cerr << "Error opening output file for chunk " << filename << "." << std::endl;
        }

    chunkFile.write(data.data(), data.size());
    chunkFile.close();  

    chunkStorage[chunkID] = filename; 
    std::cout << "Stored chunk " << chunkID << " at " << filename << std::endl;
}

void ChunkServer::storeChunkCopies(std::string& chunkName, const std::vector<std::string>& servers) {
    std::string storeCommandBase = "STORE ";
    std::string chunkPathBase = "chunks/" + chunkName;

 
    std::ifstream chunkFile(chunkPathBase, std::ios::binary | std::ios::ate);
    if (!chunkFile.is_open()) {
        std::cerr << "Failed to open file: " << chunkPathBase << " | Error: " << strerror(errno) << std::endl;
        return;
    }

    std::ifstream::pos_type fileSize = chunkFile.tellg();
    chunkFile.seekg(0, std::ios::beg);

    std::vector<char> chunkData(fileSize);
    if (!chunkFile.read(chunkData.data(), fileSize)) {
        std::cerr << "Failed to read file: " << chunkPathBase << std::endl;
        return;
    }
    chunkFile.close();

    for (size_t i = 0; i < servers.size(); ++i) {
        const auto& server = servers[i];
        size_t colonPos = server.find(':');
        if (colonPos == std::string::npos) {
            std::cerr << "Invalid format for server address: " << server << std::endl;
            continue;
        }

        std::string serverIP = server.substr(0, colonPos);
        int serverPort = std::stoi(server.substr(colonPos + 1));

        int sock = socket(AF_INET, SOCK_STREAM, 0);
        if (sock < 0) {
            std::cerr << "Socket creation error for server: " << server << std::endl;
            continue;
        }

        struct sockaddr_in serverAddr;
        serverAddr.sin_family = AF_INET;
        serverAddr.sin_port = htons(serverPort);

        if (inet_pton(AF_INET, serverIP.c_str(), &serverAddr.sin_addr) <= 0) {
            std::cerr << "Invalid address/ Address not supported for server: " << server << std::endl;
            close(sock);
            continue;
        }

        if (connect(sock, (struct sockaddr*)&serverAddr, sizeof(serverAddr)) < 0) {
            std::cerr << "Connection Failed for server: " << server << std::endl;
            close(sock);
            continue;
                }
        size_t firstUnderscorePos = chunkName.find_first_of('_');
        size_t secondUnderscorePos = chunkName.find_first_of('_', firstUnderscorePos + 1);

        std::string filename = chunkName.substr(0, firstUnderscorePos);
        std::string copyNumber = chunkName.substr(firstUnderscorePos + 1, secondUnderscorePos - firstUnderscorePos - 1);
        std::string chunk = chunkName.substr(secondUnderscorePos + 1);

        std::string serverChunkName = filename + "_" + std::to_string(i + 1) + "_" + chunk;
        std::string storeCommand = storeCommandBase + serverChunkName;
        uint32_t commandLength = storeCommand.size();
        std::cout<<storeCommand<<std::endl;

        send(sock, &commandLength, sizeof(commandLength), 0);
        send(sock, storeCommand.c_str(), storeCommand.size(), 0);

        ssize_t bytesSent = send(sock, chunkData.data(), chunkData.size(), 0);
        if (bytesSent < 0) {
            std::cerr << "Failed to send chunk data to server: " << server << std::endl;
        } else {
            std::cout << "Successfully sent chunk data as " << serverChunkName << " to server: " << server << std::endl;
        }

        close(sock);
    }
}



void ChunkServer::handleClientRequest(int clientSocket) {

    uint32_t length;
    recv(clientSocket, &length, sizeof(length), 0);  

    std::vector<char> headerBuff(length+10);
    ssize_t header_read = recv(clientSocket, headerBuff.data(), length, 0);

    if (header_read <= 0) {
        if (header_read == 0) {
            std::cerr << "Client closed connection." << std::endl;
        } else {
            std::cerr << "Error reading header from client socket." << std::endl;
        }
        return;
    }

    std::string header = std::string(headerBuff.begin(), headerBuff.begin() + header_read);
    
    if (header.starts_with("STORE ")) {
        std::cout << "Received request: STORE" << std::endl;

        auto delimiterPos = header.find(" ");  
        if (delimiterPos == std::string::npos) {
            std::cout << header << std::endl;
            std::cerr << "Invalid STORE command format." << std::endl;
            return;
        }

        std::string chunkID = header.substr(6, delimiterPos - 7);  // Extract chunkID
        std::cout << "Chunk ID: " << chunkID << std::endl;

        std::vector<char> chunkData;
        const size_t bufferSize = 1024;
        char buffer[bufferSize];

        size_t totalBytesReceived = 0;
        size_t bytesRead;
        while (true) {
            bytesRead = recv(clientSocket, buffer, bufferSize, 0);
            if (bytesRead <= 0) {
                if (bytesRead == 0) {
                    std::cerr << "Client closed connection." << std::endl;
                } else {
                    std::cerr << "Error reading chunk data from client." << std::endl;
                }
                break;
            }

            totalBytesReceived += bytesRead;
            chunkData.insert(chunkData.end(), buffer, buffer + bytesRead);
 
            if (bytesRead < bufferSize) {
                break;  
            }
        }

        std::string data(chunkData.begin(), chunkData.end());
        std::cout << "Received chunk data of size: " << data.size() << " bytes." << std::endl;
        storeChunk(chunkID, data);  

    } else if (header.starts_with("RETRIEVE ")) {
        std::cout << "Received request: RETRIEVE" << std::endl;

        std::string chunkName = header.substr(9);  
        std::cout<<header<<"\n"<<chunkName<<"\n";
        std::string chunkPath = "chunks/" + chunkName;

        std::ifstream chunkFile(chunkPath, std::ios::binary);
        if (chunkFile) {
            std::cout << "Found chunk: " << chunkName << std::endl;

            std::stringstream buffer;
            buffer << chunkFile.rdbuf();

            std::string chunkData = buffer.str();

            ssize_t bytesSent = send(clientSocket, chunkData.c_str(), chunkData.length(), 0);
            if (bytesSent < 0) {
                std::cerr << "Failed to send chunk data." << std::endl;
            } else {
                std::cout << "Sent chunk data for: " << chunkName << std::endl;
            }

        }else if(header.starts_with("PING")){
            std::string response = "PONG";
            send(clientSocket,response.c_str(), response.size(),0);
        } else {
            std::cerr << "Chunk not found: " << chunkName << std::endl;

        }

    } else if (header.find("SERVERS ") == 0) {
    std::string serversPart = header.substr(8);
    size_t spacePos = serversPart.find(' ');
    if (spacePos != std::string::npos) {
        std::string chunkName = serversPart.substr(0, spacePos);
        std::string serversList = serversPart.substr(spacePos +1);
        std::vector<std::string> servers;
        size_t commaPos = serversList.find(',');
        if (commaPos != std::string::npos) {
            servers.push_back(serversList.substr(0, commaPos));
            servers.push_back(serversList.substr(commaPos + 1));
        } else {
            servers.push_back(serversList);
        }
        storeChunkCopies(chunkName, servers);
    } else {
        std::cerr << "Invalid SERVERS message format." << std::endl;
    }
} else {
    std::cerr << "Invalid command received." << std::endl;
    std::cout << header  << std::endl;
    std::cout << "Size of header:" << std::to_string(header.size()) << std::endl;
}

}
