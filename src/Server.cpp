#include <iostream>
#include <unordered_map>
#include <map>
#include <string>
#include <hash_map>
#include <vector>
#include <memory>
#include <stdexcept>
#include "Server.hpp"

// clients request a file from the server. If it doesn't exist, throw error
std::string Server::readFile(std::string filename) {
    for (const auto& [file, data] : file_to_data) {
        if (file == filename) {
            return data;
        }
    }

    throw std::runtime_error("file does not exist -- cannot be open!");
    // return null;
}

// overwrites file or creates new file if it did not previously exist
void Server::writeFile(std::string filename, std::string new_content) {
    file_to_data[filename] = new_content;
}

// int main() {
//     Server srv;
// 
//     return 0;
// }
