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

#include <iostream>
#include <fstream>
#include <sstream>
#include <filesystem>
#include <cstring>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

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
            handleClientRequest(clientSocket); 

            close(clientSocket);
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
    masterAddr.sin_port = htons(8080);  // Master server port
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


// void ChunkServer::storeChunkCopies( std::string& chunkName, const std::vector<std::string>& servers) {
//     //std::string chunkNameCopy1 = chunkName + "_0";
//     std::string storeCommand1 = "STORE " + chunkName;
//     uint32_t length = storeCommand1.size();

//     std::cout<<servers[0] <<" | " << servers[1]<<std::endl;

//     size_t colonPos0 = servers[0].find(':');
//     if (colonPos0 == std::string::npos) {
//         std::cerr << "Invalid format for server address: " << servers[0] << std::endl;
//         return;
//     }
//     std::string server0IP = servers[0].substr(0, colonPos0);
//     int server0Port = std::stoi(servers[0].substr(colonPos0 + 1));

//     int sock1 = socket(AF_INET, SOCK_STREAM, 0);
//     if (sock1 < 0) {
//         std::cerr << "Socket creation error for server 1" << std::endl;
//         return;
//     }

//     struct sockaddr_in serverAddr1;
//     serverAddr1.sin_family = AF_INET;
//     serverAddr1.sin_port = htons(server0Port);

//     if (inet_pton(AF_INET, server0IP.c_str(), &serverAddr1.sin_addr) <= 0) {
//         std::cerr << "Invalid address/ Address not supported for server 1" << std::endl;
//         return;
//     }

//     if (connect(sock1, (struct sockaddr*)&serverAddr1, sizeof(serverAddr1)) < 0) {
//         std::cerr << "Connection Failed for server 1" << std::endl;
//         return;
//     }

//     send(sock1, &length, sizeof(length), 0);

//     send(sock1, storeCommand1.c_str(), storeCommand1.size(), 0);

//     std::string chunkPath = "chunkNode/chunks/" + chunkName;

//     if (!std::filesystem::exists(chunkPath)) {
//     std::cerr << "Error: File does not exist: " << chunkPath << std::endl;
//     return;
// }

// if (!std::filesystem::is_regular_file(chunkPath)) {
//     std::cerr << "Error: Path is not a regular file: " << chunkPath << std::endl;
//     return;
// }

//     for (const auto& entry : std::filesystem::directory_iterator("chunks/")) {
//         std::cout << "Found file: " << entry.path().string() << std::endl;
//     }

//     std::ifstream chunkFile(chunkPath, std::ios::binary);
//     if (chunkFile) {
//         std::cout << "Found chunk: " << chunkName << std::endl;

//         std::stringstream buffer;
//         buffer << chunkFile.rdbuf();

//         std::string chunkData = buffer.str();

//         ssize_t bytesSent = send(sock1, chunkData.c_str(), chunkData.length(), 0);
//         if (bytesSent < 0) {
//             std::cerr << "Failed to send chunk data." << std::endl;
//         } else {
//             std::cout << "Sent chunk data for: " << chunkName << std::endl;
//         }

//     } else {
//             std::cerr << "Failed to open file: " << chunkPath << " | Error: " << strerror(errno) << std::endl;
//             std::cout << "Chunk Path (Absolute): " << std::filesystem::absolute(chunkPath).string() << std::endl;


//     }

//     std::cout << "Store command sent to server 1: " << storeCommand1 << std::endl;
//     close(sock1);
// }


void ChunkServer::storeChunkCopies(std::string& chunkName, const std::vector<std::string>& servers) {
    std::string storeCommand1 = "STORE " + chunkName;
    uint32_t length = storeCommand1.size();

    std::cout << servers[0] << " | " << servers[1] << std::endl;

    // Parse the first server's IP and port
    size_t colonPos0 = servers[0].find(':');
    if (colonPos0 == std::string::npos) {
        std::cerr << "Invalid format for server address: " << servers[0] << std::endl;
        return;
    }
    std::string server0IP = servers[0].substr(0, colonPos0);
    int server0Port = std::stoi(servers[0].substr(colonPos0 + 1));

    // Create socket for the first server
    int sock1 = socket(AF_INET, SOCK_STREAM, 0);
    if (sock1 < 0) {
        std::cerr << "Socket creation error for server 1" << std::endl;
        return;
    }

    struct sockaddr_in serverAddr1;
    serverAddr1.sin_family = AF_INET;
    serverAddr1.sin_port = htons(server0Port);

    if (inet_pton(AF_INET, server0IP.c_str(), &serverAddr1.sin_addr) <= 0) {
        std::cerr << "Invalid address/ Address not supported for server 1" << std::endl;
        return;
    }

    if (connect(sock1, (struct sockaddr*)&serverAddr1, sizeof(serverAddr1)) < 0) {
        std::cerr << "Connection Failed for server 1" << std::endl;
        return;
    }

    // Send the STORE command
    send(sock1, &length, sizeof(length), 0);
    send(sock1, storeCommand1.c_str(), storeCommand1.size(), 0);

    // Resolve and verify chunk path
    std::string chunkPath = "chunks/" + chunkName;
    std::string absoluteChunkPath = std::filesystem::absolute(chunkPath).string();
    std::cout << "Chunk Path (Absolute): " << absoluteChunkPath << std::endl;

    if (!std::filesystem::exists(chunkPath)) {
        std::cerr << "Error: File does not exist: " << chunkPath << std::endl;
        return;
    }

    if (!std::filesystem::is_regular_file(chunkPath)) {
        std::cerr << "Error: Path is not a regular file: " << chunkPath << std::endl;
        return;
    }

    // Open the binary chunk file
    std::ifstream chunkFile(chunkPath, std::ios::binary | std::ios::ate);
    if (!chunkFile.is_open()) {
        std::cerr << "Failed to open file: " << chunkPath << " | Error: " << strerror(errno) << std::endl;
        return;
    }

    // Get file size
    std::ifstream::pos_type fileSize = chunkFile.tellg();
    chunkFile.seekg(0, std::ios::beg);

    if (fileSize <= 0) {
        std::cerr << "Error: File is empty or size is invalid: " << chunkPath << std::endl;
        return;
    }

    // Read the entire binary file into memory
    std::vector<char> chunkData(fileSize);
    if (!chunkFile.read(chunkData.data(), fileSize)) {
        std::cerr << "Failed to read file: " << chunkPath << std::endl;
        return;
    }
    chunkFile.close();

    // Send the binary data
    ssize_t bytesSent = send(sock1, chunkData.data(), chunkData.size(), 0);
    if (bytesSent < 0) {
        std::cerr << "Failed to send chunk data." << std::endl;
    } else {
        std::cout << "Sent chunk data for: " << chunkName << std::endl;
    }

    std::cout << "Store command sent to server 1: " << storeCommand1 << std::endl;

    // Close the socket
    close(sock1);
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

        } else {
            std::cerr << "Chunk not found: " << chunkName << std::endl;

            std::string errorMessage = "ERROR: Chunk " + chunkName + " not found.";
            send(clientSocket, errorMessage.c_str(), errorMessage.length(), 0);
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
