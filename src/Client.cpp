#include "Coordinator.hpp"
#include "Server.hpp"
#include <vector>
#include <cassert>
#include <iostream>

class Client {
public:
    // initiate with number of servers, a coordinator, and a vector of servers (with n_srvs servers)
    Client(int n_srvs, Coordinator coord, std::vector<Server> srvs): n_srvs(n_srvs), coord(coord), srvs(srvs) {
        // make sure correct number of servers
        assert(srvs.size() == n_srvs);
    }

    int get_n_srvs() {
        return this->n_srvs;
    }

    std::string read_file(const std::string& filename) {
        const int srv = coord.requestFile(filename);
        return srvs[srv].readFile(filename);
    }

    void write_file(const std::string& filename, const std::string& content) {
        const int srv = coord.requestFile(filename);
        srvs[srv].writeFile(filename, content);
    }

    void create_file(const std::string& filename) {
        const int srv = coord.requestFile(filename);
        srvs[srv].writeFile(filename, "");
    }

    void delete_file() {}
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

    Client clnt(2, coord, srvs);
    std::cout << clnt.get_n_srvs() << "\n";

    std::string filename = "tmp";
    clnt.write_file("tmp", "random\n");
    std::cout << clnt.read_file("tmp");

    return 0;
}
