#include <iostream>
#include <fstream>
#include <cstring>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>

#define PORT 8080
#define BUFFER_SIZE 1024

int main() {
    int sock = 0;
    struct sockaddr_in serv_addr;
    char buffer[BUFFER_SIZE] = {0};

    sock = socket(AF_INET, SOCK_STREAM, 0);

    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(PORT);

    inet_pton(AF_INET, "127.0.0.1", &serv_addr.sin_addr);

    connect(sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr));

    std::cout << "Connected to server, sending file..." << std::endl;

    std::string file_path = "/Users/abkodrogobic/UCU/DFS/image.jpg";
    std::string file_name = file_path.substr(file_path.find_last_of("/") + 1);

    send(sock, file_name.c_str(), file_name.size(), 0);

    std::ifstream infile(file_path, std::ios::binary);

    while (!infile.eof()) {
        infile.read(buffer, BUFFER_SIZE);
        ssize_t bytes_read = infile.gcount();
        send(sock, buffer, bytes_read, 0);
    }

    std::cout << "File sent successfully." << std::endl;

    infile.close();
    close(sock);

    return 0;
}
