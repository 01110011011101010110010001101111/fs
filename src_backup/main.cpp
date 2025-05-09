#include <iostream>
#include <vector>
#include <thread>
#include <mutex>
#include <chrono>
#include "Client.hpp"
#include "Server.hpp"
#include "Coordinator.hpp"

std::mutex mtx_2;


void client_operations(int id, Coordinator& coord, std::vector<Server>& srvs, int write_percentage, int total_operations) {
    Client clnt(2, coord, srvs);
    int write_count = (write_percentage * total_operations) / 100;
    int read_count = total_operations - write_count;

    for (int i = 0; i < total_operations; ++i) {
        if (i < write_count) {
            std::string filename = "tmp_" + std::to_string(id) + "_" + std::to_string(i);
            // write
            clnt.write_file(filename, "random data from client " + std::to_string(id) + " operation " + std::to_string(i) + "\n");
            // {
            //     std::lock_guard<std::mutex> lock(mtx_2);
            //     std::cout << "Client " << id << " wrote to " << filename << "\n";
            // }
        } else {
            // let the filename be something we have written
            std::string filename = "tmp_" + std::to_string(id) + "_" + std::to_string((i) % (write_count));
            // read
            clnt.read_file(filename);
            // {
            //     std::lock_guard<std::mutex> lock(mtx_2);
            //     std::cout << "Client " << id << " read: " << clnt.read_file(filename) << "\n";
            // }
        }
    }
}

int main(int argc, char* argv[]) {
    if (argc != 4) {
        std::cerr << "Usage: " << argv[0] << " <num_clients> <write_percentage> <total_operations>\n";
        return 1;
    }

    int num_clients = std::stoi(argv[1]);
    int write_percentage = std::stoi(argv[2]);
    int total_operations = std::stoi(argv[3]);

    Server srv1;
    Server srv2;
    Coordinator coord(2);
    std::vector<Server> srvs = {srv1, srv2};

    std::vector<std::thread> threads;

    auto start_time = std::chrono::high_resolution_clock::now();

    for (int i = 0; i < num_clients; ++i) {
        threads.emplace_back(client_operations, i, std::ref(coord), std::ref(srvs), write_percentage, total_operations);
    }

    // Join threads
    for (auto& thread : threads) {
        thread.join();
    }

    auto end_time = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double> duration = end_time - start_time;

    std::cout << "Total execution time: " << duration.count() << " seconds\n";

    return 0;
}
