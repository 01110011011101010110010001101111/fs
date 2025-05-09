#include <string>
#include <iostream>
#include <fstream>
#include <filesystem>

// mount location
#define LOCATION "filesystem"

class Filesystem {
public:
    int create_file(const std::string& filename) {
        std::string full_path = std::string(LOCATION) + "/" + filename;
        std::ofstream ofs(full_path);
        if (!ofs) {
            std::cerr << "Error creating file: " << full_path << std::endl;
            return -1;
        }
        ofs.close();
        return 0;
    }

    int delete_file(const std::string& filename) {
        std::string full_path = std::string(LOCATION) + "/" + filename;
        if (!std::filesystem::exists(full_path)) {
            std::cerr << "File not found: " << full_path << std::endl;
            return -1;
        }
        std::filesystem::remove(full_path);
        return 0;
    }

    int write_file(const std::string& filename, const std::string& content) {
        std::string full_path = std::string(LOCATION) + "/" + filename;
        std::ofstream ofs(full_path);
        if (!ofs) {
            std::cerr << "Error opening file for writing: " << full_path << std::endl;
            return -1;
        }
        ofs << content;
        ofs.close();
        return 0;
    }

    int read_file(const std::string& filename, std::string& content) {
        std::string full_path = std::string(LOCATION) + "/" + filename;
        if (!std::filesystem::exists(full_path)) {
            std::cerr << "File not found: " << full_path << std::endl;
            return -1;
        }
        std::ifstream ifs(full_path);
        if (!ifs) {
            std::cerr << "Error opening file for reading: " << full_path << std::endl;
            return -1;
        }
        std::getline(ifs, content, '\0');
        ifs.close();
        return 0;
    }
private:
};
