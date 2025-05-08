#include <grpcpp/grpcpp.h>
#include "Rdmafs_RPC.grpc.pb.h"

#define FUSE_USE_VERSION 31
#include <fuse.h>
#include <dirent.h>
#include <unistd.h>

#include <string>
#include <iostream>
#include <fstream>
#include <filesystem>
#include <map>
#include <cstring>

using grpc::Server;
using grpc::ServerBuilder;
using grpc::ServerContext;
using grpc::Status;
using rdmafs::Coordinator;
using rdmafs::Path;
using rdmafs::Attr;
using rdmafs::OpenRequest;
using rdmafs::OpenReply;
using rdmafs::ReadRequest;
using rdmafs::ReadChunk;
using rdmafs::WriteChunk;
using rdmafs::MessageStatus;

class CoordinatorServiceImpl final : public rdmafs::Coordinator::Service {
 public:
  explicit CoordinatorServiceImpl(std::string root) : base_(std::move(root)) {}

  grpc::Status GetAttr(grpc::ServerContext*, const rdmafs::Path* req,
                       rdmafs::Attr* out) override {
    struct stat st;
    if (lstat((base_ + req->path()).c_str(), &st) == -1)
      return grpc::Status(grpc::StatusCode::UNKNOWN, strerror(errno));

    out->set_stat(reinterpret_cast<char*>(&st), sizeof(st));
    return grpc::Status::OK;
  }

  grpc::Status Open(grpc::ServerContext*, const rdmafs::OpenRequest* req,
                    rdmafs::OpenReply* rep) override {
    int fd = open((base_ + req->path()).c_str(), req->flags());
    if (fd == -1)
      return grpc::Status(grpc::StatusCode::UNKNOWN, strerror(errno));

    rep->set_fh(static_cast<uint64_t>(fd));
    return grpc::Status::OK;
  }

  // …implement the rest
 private:
  std::string base_;
};

int main() {
  const std::string addr{"0.0.0.0:50051"};
  CoordinatorServiceImpl svc("/home/alex/workspace/fs_mount1");

  grpc::ServerBuilder builder;
  builder.AddListeningPort(addr, grpc::InsecureServerCredentials());
  builder.RegisterService(&svc);
  std::unique_ptr<grpc::Server> server = builder.BuildAndStart();
  std::cout << "Coordinator listening on " << addr << '\n';
  server->Wait();
}
