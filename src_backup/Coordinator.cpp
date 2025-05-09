#include <iostream>
#include <unordered_map>
#include <unordered_map>
#include <string>
#include <hash_map>
#include <vector>
#include <memory>
#include "Coordinator.hpp"

#define NUM_SERVERS 2

Coordinator::Coordinator (int n_srvs): n_srvs(n_srvs) {}

int Coordinator::requestFile(const std::string& filename) {
    // locks, will automatically unlock when out of scope
    // std::lock_guard<std::mutex> lock(mtx);

    auto it = file_server_data.find(filename);
    if (it != file_server_data.end()) {
        return it->second;
    }
    
    std::hash<std::string> hasher;
    size_t hash = hasher(filename);
    int server_num = hash % this->n_srvs;
    file_server_data[filename] = server_num;
    return server_num;
}

// int main() {
//     Coordinator coord(2);
//     return 0;
// }
