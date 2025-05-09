#ifndef COORDINATOR_H
#define COORDINATOR_H

#include <string>
#include <unordered_map>
#include <mutex>

class Coordinator {
public:
    Coordinator(int n_srvs);
    int requestFile(const std::string& filename);

    // NOTE: mutex NOT copied
    Coordinator(const Coordinator& other) : n_srvs(other.n_srvs), file_server_data(other.file_server_data) {}

private:
    int n_srvs;
    std::unordered_map<std::string, int> file_server_data;
    // std::mutex mtx;
    bool is_primary;
};

#endif // COORDINATOR_H

