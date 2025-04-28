#include "Coordinator.hpp"
#include "Server.hpp"
#include <vector>
#include <cassert>

class Client {
public:
    // initiate with number of servers, a coordinator, and a vector of servers (with n_srvs servers)
    Client(int n_srvs, Coordinator coord, std::vector<Server> srvs): n_srvs(n_srvs), coord(coord), srvs(srvs) {
        // make sure correct number of servers
        assert(srvs.size() == n_srvs);
    }

    // std::string read_file() {
    //     
    // }

    // std::string write_file() {

    // }

    // std::string create_file() {

    // }

    // std::string delete_file() {

    // }
private:
    Coordinator coord;
    int n_srvs;
    std::vector<Server> srvs;
};

int main() {
    Server srv1;
    Server srv2;
    Coordinator coord(2);
    std::vector<Server> srvs;
    srvs.push_back(srv1);
    srvs.push_back(srv2);

    Client(2, coord, srvs);

    return 0;
}
