#include <iostream>
#include <unordered_map>
#include <map>
#include <string>
#include <hash_map>
#include <vector>
#include <memory>

// Server which returns the files!
class Server {
public:
    // clients request a file from the coordinator. If it doesn't exist, hash the filename and put it into the corresponding server 
    std::string requestFile(std::string filename);

private:
    // Map of [file] -> server_data
    std::map<std::string, std::string> file_to_data;
}

std::string requestFile(std::string filename) {
    for (const auto& [file, data] : file_to_data) {
        if (file == filename) {
            return data;
        }
    }

    file_to_data[filename] = "";

    return file_to_data[filename];

}
