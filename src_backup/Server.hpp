#ifndef SERVER_HPP
#define SERVER_HPP

#include <string>
#include <map>

class Server {
public:
    std::string readFile(const std::string filename);
    void writeFile(std::string filename, std::string new_content);
private:
    std::map<std::string, std::string> file_to_data;
};

#endif // SERVER_HPP
