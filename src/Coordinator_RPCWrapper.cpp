#include <grpcpp/grpcpp.h>
#include "Coordinator_RPC.grpc.pb.h"
#include "Coordinator.hpp"
#include <iostream>

using grpc::Server;
using grpc::ServerBuilder;
using grpc::ServerContext;
using grpc::Status;
using contact::Contact;
using contact::Request;
using contact::Reply;

class ContactServiceImplementation final : public Contact::Service {
	public: 
		ContactServiceImpl(int n_srvs) : coordinator


}
