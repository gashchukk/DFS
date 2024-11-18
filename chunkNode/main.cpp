#include "includes/ChunkServerClient.h"
#include "includes/ChunkServer.h"
#include <thread>
#include <chrono>
#include <filesystem>
#include <unordered_set>
#include <iostream>

namespace fs = std::filesystem;

int main() {
    std::unordered_set<std::string> processedChunks; 
    std::thread t1([&]() {
        ChunkServerClient chunkClient("127.0.0.1", 8080);
        while (true) {
            for (const auto& entry : fs::directory_iterator("chunks/")) {
                std::string chunkPath = entry.path().string();

                if (processedChunks.find(chunkPath) == processedChunks.end()) {
                    std::string chunkName = entry.path().filename().string();
                    std::cout << "New chunk detected: " << chunkName << std::endl;

                    chunkClient.sendHeartbeat(chunkName);

                    processedChunks.insert(chunkPath);
                }
            }
            std::this_thread::sleep_for(std::chrono::seconds(5)); // Check for new chunks every 5 seconds
        }
    });

    // Thread to start the Chunk Server
    std::thread t2([&]() {
        ChunkServer chunkServer(8081);
        chunkServer.start();
    });

    t1.join();
    t2.join();
    return 0;
}
