#include "MetadataServer.h"

MetadataServer::MetadataServer(int port) {
    serverSocket = socket(AF_INET, SOCK_STREAM, 0);
    sockaddr_in serverAddr;
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_addr.s_addr = INADDR_ANY;
    serverAddr.sin_port = htons(port);
    
    bind(serverSocket, (struct sockaddr*)&serverAddr, sizeof(serverAddr));
    listen(serverSocket, 5);
}

void MetadataServer::start() {
    std::cout << "Metadata Server is running..." << std::endl;
    while (true) {
        int clientSocket = accept(serverSocket, nullptr, nullptr);
        handleClient(clientSocket);
        close(clientSocket);
    }
}

void MetadataServer::handleClient(int clientSocket) {
    char buffer[1024] = {0};
    read(clientSocket, buffer, sizeof(buffer));
    std::string command(buffer);
    
    std::cout << "Received command: " << command << std::endl; // Debugging output

    std::istringstream iss(command);
    std::string action, filename, data;
    iss >> action >> filename;
    
    if (action == "CREATE") {
        fileMetadata[filename] = "";
        std::string response = "File created: " + filename;
        send(clientSocket, response.c_str(), response.size(), 0);
    } else if (action == "WRITE") {
        std::getline(iss, data);
        fileMetadata[filename] = data;
        std::string response = "Data written to: " + filename;
        send(clientSocket, response.c_str(), response.size(), 0);
    } else if (action == "READ") {
        std::string response = "Data in " + filename + ": " + fileMetadata[filename];
        send(clientSocket, response.c_str(), response.size(), 0);
    } else if (action == "DELETE") {
        fileMetadata.erase(filename);
        std::string response = "File deleted: " + filename;
        send(clientSocket, response.c_str(), response.size(), 0);
    } else {
        std::string response = "Unknown command";
        send(clientSocket, response.c_str(), response.size(), 0);
    }
}
