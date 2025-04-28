#ifndef FILESYSTEM_HPP
#define FILESYSTEM_HPP

#include <string>

class Filesystem {
public:
    int create_file(const std::string& filename);
    int delete_file(const std::string& filename);
    int write_file(const std::string& filename, const std::string& content);
    int read_file(const std::string& filename, std::string& content);
};

#endif // FILESYSTEM_HPP
