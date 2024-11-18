#include "includes/Client.h"
#include <iostream>
#include <string>
#include <fstream>

int main() {
    std::string masterIP = "127.0.0.1";
    int masterPort = 8080;

    Client client(masterIP, masterPort);

    std::string action;
    std::cout << "Enter action (STORE or RETRIEVE or LIST): ";
    std::cin >> action;

    if (action == "STORE") {
        std::string  filePath;

        std::cout << "Enter file path to read data from: ";
        std::cin >> filePath;

        
        client.writeFile(filePath); 
        std::cout << "Data stored successfully." << std::endl;

    } else if (action == "RETRIEVE") {
        std::cout<<"retrieve1";
    } else {
        std::cout << "Invalid action specified. Please enter STORE or RETRIEVE." << std::endl;
    }

    return 0;
}
