#ifndef SERVER_HPP
#define SERVER_HPP

#include <string>
#include <map>
#include <stdexcept>
#include <vector>
#include <sstream>

class Server {
public:
    std::string readFile(const std::string& path);
    void writeFile(const std::string& path, const std::string& new_content);
private:
    struct Directory {
        std::map<std::string, Directory> subdirectories;
        std::string file_content;
        bool is_file = false;
    };

    Directory root;
    Directory* navigateTo(const std::string& path);
    std::vector<std::string> splitPath(const std::string& path);
};

#endif // SERVER_HPP
