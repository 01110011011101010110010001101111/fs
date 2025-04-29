#include <iostream>
#include <unordered_map>
#include <map>
#include <string>
#include <hash_map>
#include <vector>
#include <memory>
#include <stdio.h>
#include "../src/Client.hpp"
#include "../src/Coordinator.hpp"
#include "../src/Server.hpp"

int main() {
    Coordinator coordinator(2);
    Server server1;
    Server server2;
    std::vector<Server> servers;
    servers.push_back(server1);
    servers.push_back(server2);
    Client client(2, coordinator, servers);

    // first ask to write "1" to file "a"
    client.write_file("a", "1");

    // then read file "a"
    std::string read_value = client.read_file("a");
    if (read_value != "1") {
        printf("Client did not read correct value\n");
    }

    // create a file "b"
    client.create_file("b");

    // ensure "b" is an empty file
    std::string empty_value = client.read_file("b");
    if (empty_value != "") {
        printf("Client expected an empty file\n");
    }

    return 0;
}
