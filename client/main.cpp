#include "includes/Client.h"
#include <iostream>
#include <string>
#include <fstream>

int main() {
    std::string masterIP = "127.0.0.1";
    int masterPort = 8080;

    Client client(masterIP, masterPort);
    while(true){
        std::string action;
        std::cout << "Enter action (STORE or RETRIEVE or LIST or EXIT): ";
        std::cin >> action;

        if (action == "STORE") {
            std::string  filePath;

            std::cout << "Enter file path to read data from: ";
            std::cin >> filePath;

            
            client.writeFile(filePath); 
            std::cout << "Data stored successfully." << std::endl;

        } else if (action == "RETRIEVE") {
            std::string fileName;
            std::cout << "Enter file name to read: "<<std::endl;
            std::cin >> fileName;

            client.readfile(fileName);
            std::cout<<"File read successfully"<<std::endl;
            
        } else if (action == "LIST"){
            client.listFiles();
        } else if (action == "EXIT"){
            break;
        }else {
            std::cout << "Invalid action specified. Please enter STORE or RETRIEVE." << std::endl;
        }
    }
    return 0;
}
