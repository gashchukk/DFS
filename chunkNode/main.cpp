#include "includes/ChunkServerClient.h"
#include "includes/ChunkServer.h"
#include <thread>
#include <chrono>

int main() {
    std::thread t1([&]() {
        ChunkServerClient chunkClient("127.0.0.1", 8080);
        while (true) {
            chunkClient.sendHeartbeat();
            std::this_thread::sleep_for(std::chrono::seconds(5)); // Send heartbeat every 5 seconds
        }
    });
    std::thread t2([&](){
        ChunkServer chunkServer(8081);
        chunkServer.start();
    });
    

    t1.join();
    t2.join();
    return 0;
}