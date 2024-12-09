#include "includes/ChunkServerClient.h"
#include "includes/ChunkServer.h"
#include <thread>
#include <chrono>
#include <filesystem>
#include <unordered_set>
#include <iostream>
#include <string>

namespace fs = std::filesystem;

int main(int argc, char* argv[]) {
    if (argc != 2) {
        std::cerr << "Usage: " << argv[0] << " <port>" << std::endl;
        return 1;
    }

    int port = std::stoi(argv[1]);

    std::string directory = "chunks";
    if (!fs::exists(directory)) {
        try {
            fs::create_directory(directory);
        } catch (const std::filesystem::filesystem_error& e) {
            std::cerr << "Error creating 'chunks' directory: " << e.what() << std::endl;
            return 1;
        }
    }

    std::unordered_set<std::string> processedChunks;

    std::thread t1([&]() {
    ChunkServerClient chunkClient("127.0.0.1", 8080, port);
    while (true) {
        for (const auto& entry : fs::directory_iterator("chunks/")) {
            std::string chunkPath = entry.path().string();

            if (processedChunks.find(chunkPath) == processedChunks.end()) {
                std::string chunkName = entry.path().filename().string();

                chunkClient.sendHeartbeat(chunkName);

                processedChunks.insert(chunkPath);
            }
        }
        std::this_thread::sleep_for(std::chrono::seconds(10));  // Adjusted sleep interval to 10 seconds
    }
});


    std::thread t2([&]() {
        ChunkServer chunkServer(port); 
        chunkServer.start();
    });

    t1.join();
    t2.join();

    return 0;
}
