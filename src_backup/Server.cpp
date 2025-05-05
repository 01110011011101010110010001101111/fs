#include <iostream>
#include <unordered_map>
#include <map>
#include <string>
#include <hash_map>
#include <vector>
#include <memory>
#include <stdexcept>
#include "Server.hpp"

std::vector<std::string> Server::splitPath(const std::string& path) {
    std::vector<std::string> components;
    std::stringstream ss(path);
    std::string item;

    while (std::getline(ss, item, '/')) {
        if (!item.empty()) {
            components.push_back(item);
        }
    }

    return components;
}

Server::Directory* Server::navigateTo(const std::string& path) {
    Directory* current = &root;
    auto components = splitPath(path);

    for (const auto& component : components) {
        if (current->subdirectories.find(component) == current->subdirectories.end()) {
            return nullptr;
        }
        current = &current->subdirectories[component];
    }

    return current;
}

// clients request a file from the server. If it doesn't exist, throw error
std::string Server::readFile(const std::string& path) {
    Directory* dir = navigateTo(path);
    if (dir && dir->is_file) {
        return dir->file_content;
    }

    throw std::runtime_error("file does not exist -- cannot be opened!");
}

// overwrites file or creates new file if it did not previously exist
void Server::writeFile(const std::string& path, const std::string& new_content) {
    Directory* dir = navigateTo(path);
    if (dir) {
        dir->file_content = new_content;
        dir->is_file = true; 
    } else {
        Directory* current = &root;
        auto components = splitPath(path);
        for (size_t i = 0; i < components.size(); ++i) {
            const auto& component = components[i];
            if (current->subdirectories.find(component) == current->subdirectories.end()) {
                current->subdirectories[component] = Directory();
            }
            current = &current->subdirectories[component];
        }
        current->file_content = new_content;
        current->is_file = true;
    }
}

// int main() {
//     Server srv;
// 
//     return 0;
// }
