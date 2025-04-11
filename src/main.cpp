#include "Client.h"
#include "MetadataServer.h"
#include <thread>

void runServer(int port) {
    MetadataServer server(port);
    server.start();
}

int main(int argc, char* argv[]) {
    if (argc < 2) {
        std::cerr << "Usage: " << argv[0] << " <server|client>" << std::endl;
        return 1;
    }

    if (std::string(argv[1]) == "server") {
        runServer(8080); // Start the metadata server on port 8080
    } else if (std::string(argv[1]) == "client") {
        Client client("127.0.0.1", 8080); // Connect to the server
        client.createFile("example.txt");
        client.writeFile("example.txt", "Hello, Distributed File System!");
        client.readFile("example.txt");
        client.deleteFile("example.txt");
    } else {
        std::cerr << "Invalid argument. Use 'server' or 'client'." << std::endl;
        return 1;
    }

    return 0;
}
