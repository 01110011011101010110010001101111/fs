#include <iostream>
#include <unordered_map>
#include <map>
#include <string>
#include <hash_map>
#include <vector>
#include <memory>
#include <stdio.h>
#include <stdexcept>
#include "../src/Client.hpp"
#include "../src/Coordinator.hpp"
#include "../src/Server.hpp"

int main() {
    Server server;

    // try writing then reading from file
    server.writeFile("a", "1");
    std::string output = server.readFile("a");
    if (output != "1") {
        printf("server did not write file correctly, expected 1");
    } 

    // try reading from a file that does not exist 
    bool error = false;
    try { 
        server.readFile("b")
    } catch (const std::runtime_error& e) {
        error = true;
    }
    if !error {
        printf("server allowed read from nonexistent file");
    }

    // create that file and try again 
    server.writeFile("b", "2");
    std::string output = server.readFile("b");
    if (output != "2") {
        printf("server did not write file correctly, expected 2");
    } 

    return 0
}