#ifndef SERVER_HPP
#define SERVER_HPP

#include <string>

class Server {
public:
    std::string requestFile(const std::string filename);
    void writeFile(std::string filename, std::string new_content);
};

#endif // SERVER_HPP
