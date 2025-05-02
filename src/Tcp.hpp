/**
 * g++ -Wall -std=c++20 Tcp.cpp -o tcp
 */
// #include <boost/asio.hpp>

#include <ifaddrs.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <net/if.h> 
#include <sys/socket.h>
#include <unistd.h>
#include <iostream>

std::string get_local_ip(const std::string& interface_name) {
    struct ifaddrs *ifaddr, *ifa;
    char ip[INET_ADDRSTRLEN];

    if (getifaddrs(&ifaddr) == -1) {
        perror("getifaddrs");
        return "";
    }

    for (ifa = ifaddr; ifa != nullptr; ifa = ifa->ifa_next) {
        if (!ifa->ifa_addr) continue;

        if (ifa->ifa_addr->sa_family == AF_INET &&
            !(ifa->ifa_flags & IFF_LOOPBACK) && std::string(ifa->ifa_name) == interface_name ) {

            struct sockaddr_in *sa = (struct sockaddr_in *)ifa->ifa_addr;
            if (inet_ntop(AF_INET, &(sa->sin_addr), ip, INET_ADDRSTRLEN)) {
                freeifaddrs(ifaddr);
                return std::string(ip);
            }
        }
    }

    freeifaddrs(ifaddr);
    return "";
}

class TcpReplicator {
    int sockfd = -1;
public:
    bool connect(const std::string& remote_addr, int port) {
        sockfd = socket(AF_INET, SOCK_STREAM, 0);
        sockaddr_in serv_addr{};
        serv_addr.sin_family = AF_INET;
        serv_addr.sin_port = htons(port);
        inet_pton(AF_INET, remote_addr.c_str(), &serv_addr.sin_addr);
        return ::connect(sockfd, (sockaddr*)&serv_addr, sizeof(serv_addr)) == 0;
    }

    bool send(const void* data, size_t size) {
        ssize_t sent = ::send(sockfd, data, size, 0);
        return sent == static_cast<ssize_t>(size);
    }

    void close() {
        if (sockfd != -1) ::close(sockfd);
        sockfd = -1;
    }
};


// int main(){
//     std::cout << get_local_ip("ib0") << std::endl;
// }