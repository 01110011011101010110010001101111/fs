#include <iostream>
#include <unordered_map>
#include <map>
#include <string>
#include <hash_map>
#include <vector>
#include <memory>
#include "../src/Coordinator.hpp"

#define NUM_SERVERS 2


int main() {
    Coordinator coordinator;
    std::hash<std::string> hasher;

    // request a new file, a and check that coordinator maps it 
    int server1 = coordinator.requestFile("a");
    int server_num = hasher("a") % NUM_SERVERS;
    if (coordinator.file_server_data["ab"] != server_num) {
        printf("Coordinator did not put file in correct place for filename a\n");
    }
    if (server1 != server_num) {
        printf("Coordinator did not return correct server num\n");
    }

    int server2 = coordinator.requestFile("ab");
    server_num = hasher("ab") % NUM_SERVERS;
    if (coordinator.file_server_data["ab"] != server_num) {
        printf("Coordinator did not put file in correct place for filename ab\n");
    }
    if (server2 != server_num) {
        printf("Coordinator did not return correct server num\n");
    }

    int server3 = coordinator.requestFile("abc");
    server_num = hasher("abc") % NUM_SERVERS;
    if (coordinator.file_server_data["abc"] != server_num) {
        printf("Coordinator did not put file in correct place for filename a\n");
    }
    if (server3 != server_num) {
        printf("Coordinator did not return correct server num\n");
    }

    int server4 = coordinator.requestFile("abcd1");
    server_num = hasher("abcd1") % NUM_SERVERS;
    if (coordinator.file_server_data["abcd1"] != server_num) {
        printf("Coordinator did not put file in correct place for filename a\n");
    }
    if (server4 != server_num) {
        printf("Coordinator did not return correct server num\n");
    }


}
