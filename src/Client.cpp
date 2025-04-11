#include "Client.h"
#include <arpa/inet.h> // Include for inet_pton
#include <sys/socket.h> // Include for socket functions
#include <unistd.h> // Include for close()

Client::Client(const std::string& serverAddress, int port) : serverAddress(serverAddress), port(port) {}

void Client::sendCommand(const std::string& command) {
    int sock = socket(AF_INET, SOCK_STREAM, 0);
    sockaddr_in serverAddr;
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(port);
    inet_pton(AF_INET, serverAddress.c_str(), &serverAddr.sin_addr);

    connect(sock, (struct sockaddr*)&serverAddr, sizeof(serverAddr));
    send(sock, command.c_str(), command.size(), 0);

    char buffer[1024] = {0};
    read(sock, buffer, sizeof(buffer));
    std::cout << buffer << std::endl;

    close(sock);
}

void Client::createFile(const std::string& filename) {
    sendCommand("CREATE " + filename);
}

void Client::writeFile(const std::string& filename, const std::string& data) {
    sendCommand("WRITE " + filename + " " + data);
}

void Client::readFile(const std::string& filename) {
    sendCommand("READ " + filename);
}

void Client::deleteFile(const std::string& filename) {
    sendCommand("DELETE " + filename);
}
