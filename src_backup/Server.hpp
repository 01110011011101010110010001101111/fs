#ifndef SERVER_HPP
#define SERVER_HPP

#include <string>
#include <map>
#include <stdexcept>
#include <vector>
#include <sstream>
#include <mutex>

class Server {
public:

    Server() : root() {}
    Server(const Server& other)
        : root(other.root) {}
    std::string readFile(const std::string& path);
    void writeFile(const std::string& path, const std::string& new_content);

private:
    struct Directory {
        std::map<std::string, Directory> subdirectories;
        std::string file_content;
        bool is_file = false;

        Directory(const Directory& other)
            : subdirectories(other.subdirectories),
              file_content(other.file_content),
              is_file(other.is_file) {}

        Directory() : subdirectories(), file_content(""), is_file(false) {}
    };

    Directory root;
    Directory* navigateTo(const std::string& path);
    std::vector<std::string> splitPath(const std::string& path);
    std::mutex mtx;
};

#endif // SERVER_HPP
