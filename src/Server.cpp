#include <iostream>
#include <unordered_map>
#include <map>
#include <string>
#include <hash_map>
#include <vector>
#include <memory>
#include <stdexcept>

// Server which returns the files!
class Server {
public:
    // clients request a file from the server. If it doesn't exist, throw error
    std::string readFile(std::string filename) {
        for (const auto& [file, data] : file_to_data) {
            if (file == filename) {
                return data;
            }
        }

        throw std::runtime_error("file does not exist -- cannot be open!");
    }

    // overwrites file or creates new file if it did not previously exist
    void writeFile(std::string filename, std::string new_content) {
        file_to_data[filename] = new_content;
    }



private:
    // Map of [file] -> server_data
    std::map<std::string, std::string> file_to_data;
};

int main() {
    Server srv;

    return 0;
}
