#include <iostream>
#include <unordered_map>
#include <string>
#include <vector>
#include <memory>

class File {
public:
    File(const std::string& name) : name(name) {}

    void write(const std::string& data) {
        content = data;
    }

    std::string read() const {
        return content;
    }

    const std::string& getName() const {
        return name;
    }

private:
    std::string name;
    std::string content;
};

class FileSystem {
public:
    bool createFile(const std::string& name) {
        if (files.find(name) != files.end()) {
            std::cerr << "File already exists: " << name << std::endl;
            return false;
        }
        files[name] = std::make_shared<File>(name);
        return true;
    }

    bool deleteFile(const std::string& name) {
        if (files.erase(name) == 0) {
            std::cerr << "File not found: " << name << std::endl;
            return false;
        }
        return true;
    }

    bool writeFile(const std::string& name, const std::string& data) {
        auto it = files.find(name);
        if (it == files.end()) {
            std::cerr << "File not found: " << name << std::endl;
            return false;
        }
        it->second->write(data);
        return true;
    }

    std::string readFile(const std::string& name) {
        auto it = files.find(name);
        if (it == files.end()) {
            std::cerr << "File not found: " << name << std::endl;
            return "";
        }
        return it->second->read();
    }

    void listFiles() const {
        std::cout << "Files in the file system:" << std::endl;
        for (const auto& pair : files) {
            std::cout << " - " << pair.first << std::endl;
        }
    }

private:
    std::unordered_map<std::string, std::shared_ptr<File>> files;
};

int main() {
    FileSystem fs;

    // Create files
    fs.createFile("file1.txt");
    fs.createFile("file2.txt");

    // List files
    fs.listFiles();

    // Write to a file
    fs.writeFile("file1.txt", "Hello, World!");

    // Read from a file
    std::cout << "Contents of file1.txt: " << fs.readFile("file1.txt") << std::endl;

    // Delete a file
    fs.deleteFile("file2.txt");

    // List files again
    fs.listFiles();

    return 0;
}
