#ifndef METADATASERVER_H
#define METADATASERVER_H

#include <unordered_map>
#include <string>
#include <vector>
#include <iostream>
#include <fstream>
#include <sstream>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>

class MetadataServer {
public:
    MetadataServer(int port);
    void start();
    
private:
    void handleClient(int clientSocket);
    std::unordered_map<std::string, std::string> fileMetadata; // filename -> data
    int serverSocket;
};

#endif // METADATASERVER_H
