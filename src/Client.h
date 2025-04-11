#ifndef CLIENT_H
#define CLIENT_H

#include <string>
#include <iostream>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>

class Client {
public:
    Client(const std::string& serverAddress, int port);
    void createFile(const std::string& filename);
    void writeFile(const std::string& filename, const std::string& data);
    void readFile(const std::string& filename);
    void deleteFile(const std::string& filename);

private:
    std::string serverAddress;
    int port;
    void sendCommand(const std::string& command);
};

#endif // CLIENT_H
