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
    // create a coordinator with n_srvs
    Coordinator (int n_srvs): n_srvs(n_srvs) {}

    // clients request a file from the coordinator. If it doesn't exist, hash the filename and put it into the corresponding server 
    int requestFile(std::string filename) {
        for (const auto& [file, server] : file_server_data) {
            if (file == filename) {
                return server;
            }
        }

        std::hash<std::string> hasher;
        size_t hash = hasher(filename);
        int server_num = hash % this->n_srvs;
        file_server_data[filename] = server_num;
        return server_num;
    }

private:
    // Map of [file] -> server
    int n_srvs;
    std::map<std::string, int> file_server_data;
};



int main() {
    Coordinator coord(2);
    return 0;
}
