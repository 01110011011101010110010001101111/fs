#include "Coordinator.hpp"
#include "Server.hpp"
#include <vector>
#include <cassert>
#include <iostream>
#include <thread>

class Client {
public:
    // initiate with number of servers, a coordinator, and a vector of servers (with n_srvs servers)
    Client(int n_srvs, Coordinator coord, std::vector<Server> srvs);

    int get_n_srvs();

    std::string read_file(const std::string& filename); 

    void write_file(const std::string& filename, const std::string& content);

    void create_file(const std::string& filename);

    void delete_file();
private:
    Coordinator coord;
    int n_srvs;
    std::vector<Server> srvs;
};
