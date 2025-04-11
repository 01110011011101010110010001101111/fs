#include <iostream>
#include <chrono>
#include "FileSystem.h"

void benchmarkFileSystem(FileSystem& fs, int numFiles) {
    // Measure file creation time
    auto start = std::chrono::high_resolution_clock::now();
    for (int i = 0; i < numFiles; ++i) {
        fs.createFile("file" + std::to_string(i) + ".txt");
    }
    auto end = std::chrono::high_resolution_clock::now();
    std::cout << "Time taken to create " << numFiles << " files: "
              << std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count() << " ms" << std::endl;

    // Measure file writing time
    start = std::chrono::high_resolution_clock::now();
    for (int i = 0; i < numFiles; ++i) {
        fs.writeFile("file" + std::to_string(i) + ".txt", "This is some test data for file " + std::to_string(i));
    }
    end = std::chrono::high_resolution_clock::now();
    std::cout << "Time taken to write to " << numFiles << " files: "
              << std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count() << " ms" << std::endl;

    // Measure file reading time
    start = std::chrono::high_resolution_clock::now();
    for (int i = 0; i < numFiles; ++i) {
        fs.readFile("file" + std::to_string(i) + ".txt");
    }
    end = std::chrono::high_resolution_clock::now();
    std::cout << "Time taken to read " << numFiles << " files: "
              << std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count() << " ms" << std::endl;

    // Measure file deletion time
    start = std::chrono::high_resolution_clock::now();
    for (int i = 0; i < numFiles; ++i) {
        fs.deleteFile("file" + std::to_string(i) + ".txt");
    }
    end = std::chrono::high_resolution_clock::now();
    std::cout << "Time taken to delete " << numFiles << " files: "
              << std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count() << " ms" << std::endl;
}

int main() {
    FileSystem fs;
    const int numFiles = 1000; // Number of files to benchmark

    benchmarkFileSystem(fs, numFiles);

    return 0;
}

