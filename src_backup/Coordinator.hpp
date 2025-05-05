#ifndef COORDINATOR_H
#define COORDINATOR_H

#include <string>
#include <unordered_map>

class Coordinator {
public:
    Coordinator(int n_srvs);
    int requestFile(const std::string& filename);

private:
    int n_srvs;
    std::unordered_map<std::string, int> file_server_data;
};

#endif // COORDINATOR_H

