#include "includes/ChunkServerClient.h"
#include <thread>
#include <chrono>

int main() {
    ChunkServerClient chunkClient("127.0.0.1", 8080);

    while (true) {
        // chunkClient.sendHeartbeat();
        std::this_thread::sleep_for(std::chrono::seconds(5)); // Send heartbeat every 5 seconds
    }

    return 0;
}
