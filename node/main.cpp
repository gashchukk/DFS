#include <iostream>
#include <fstream>
#include <cstring>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>

#define PORT 8080
#define BUFFER_SIZE 1024

int main() {
    int server_fd, new_socket;
    struct sockaddr_in address;
    int addrlen = sizeof(address);
    char buffer[BUFFER_SIZE] = {0};

    server_fd = socket(AF_INET, SOCK_STREAM, 0);

    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(PORT);

    bind(server_fd, (struct sockaddr *)&address, sizeof(address));
    listen(server_fd, 3);

    std::cout << "Waiting for connection..." << std::endl;

    new_socket = accept(server_fd, (struct sockaddr *)&address, (socklen_t*)&addrlen);

    std::cout << "Connected" << std::endl;

    ssize_t bytes_read = read(new_socket, buffer, BUFFER_SIZE);
    std::string file_name(buffer, bytes_read);

    std::cout << "Receiving file: " << file_name << std::endl;

    std::ofstream outfile(file_name, std::ios::binary);

    while ((bytes_read = read(new_socket, buffer, BUFFER_SIZE)) > 0) {
        outfile.write(buffer, bytes_read);
    }

    std::cout << "File received" << std::endl;

    outfile.close();
    close(new_socket);
    close(server_fd);

    return 0;
}
