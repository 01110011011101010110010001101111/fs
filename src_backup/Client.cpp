#include "Coordinator.hpp"
#include "Server.hpp"
#include <vector>
#include <cassert>
#include <iostream>
#include <thread>
#include "Client.hpp"

// initiate with number of servers, a coordinator, and a vector of servers (with n_srvs servers)
Client::Client(int n_srvs, Coordinator coord, std::vector<Server> srvs): n_srvs(n_srvs), coord(coord), srvs(srvs) {
    // make sure correct number of servers
    assert(srvs.size() == n_srvs);
}

int Client::get_n_srvs() {
    return this->n_srvs;
}

std::string Client::read_file(const std::string& filename) {
    const int srv = coord.requestFile(filename);
    return srvs[srv].readFile(filename);
}

void Client::write_file(const std::string& filename, const std::string& content) {
    const int srv = coord.requestFile(filename);
    srvs[srv].writeFile(filename, content);
}

void Client::create_file(const std::string& filename) {
    const int srv = coord.requestFile(filename);
    srvs[srv].writeFile(filename, "");
}

void Client::delete_file() {}
