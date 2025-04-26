#include <iostream>
#include <unordered_map>
#include <map>
#include <string>
#include <hash_map>
#include <vector>
#include <memory>

#define NUM_SERVERS 2

// Coordinator keeps track of which servers have which files
class Coordinator {
public:
    // clients request a file from the coordinator. If it doesn't exist, hash the filename and put it into the corresponding server 
    int requestFile(std::string filename);

private:
    // Map of [file] -> server
    std::map<std::string, int> file_server_data;
}

int Coordinator::requestFile(std::string filename) {
    for (const auto& [file, server] : file_server_data) {
        if (file == filename) {
            return server 
        }
    }

    hash<string> hasher;
    size_t hash = hasher(filename);
    int server_num = hash % NUM_SERVERS;
    return server_num
}



