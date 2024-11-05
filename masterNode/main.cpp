#include "includes/MasterNodeServer.hpp"

int main() {
    MasterNodeServer masterServer(8080); 
    masterServer.start();
    return 0;
}
