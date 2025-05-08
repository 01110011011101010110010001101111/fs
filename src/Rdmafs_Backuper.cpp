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

using rdmafs::Backuper;
using rdmafs::Path;
using rdmafs::Attr;
using rdmafs::OpenRequest;
using rdmafs::OpenReply;
using rdmafs::ReadRequest;
using rdmafs::ReadChunk;
using rdmafs::WriteChunk;
using rdmafs::MessageStatus;

int mknod_wrapper(int dirfd, const char *path, const char *link,
	int mode, dev_t rdev) {
	int res;

	if (S_ISREG(mode)) {
		res = openat(dirfd, path, O_CREAT | O_EXCL | O_WRONLY, mode);
		if (res >= 0)
			res = close(res);
	} else if (S_ISDIR(mode)) {
		res = mkdirat(dirfd, path, mode);
	} else if (S_ISLNK(mode) && link != NULL) {
		res = symlinkat(link, dirfd, path);
	} else if (S_ISFIFO(mode)) {
		res = mkfifoat(dirfd, path, mode);
	} else {
		res = mknodat(dirfd, path, mode, rdev);
	}

	return res;
}

class BackuperServiceImpl final : public rdmafs::Backuper::Service {
 public:
  explicit BackuperServiceImpl(std::string root) : base_(std::move(root)) {}

  grpc::Status GetAttr(grpc::ServerContext*, const rdmafs::Path* req,
                       rdmafs::Attr* reply) override {
    std::lock_guard<std::mutex> lock(global_mutex_);
    struct stat st;
    if(lstat(full_path(req->path()).c_str(), &st) == -1) {
      reply->set_return_code(-errno);
      return grpc::Status::OK;
    }
    reply->mutable_attr()->set_stat(reinterpret_cast<char*>(&st), sizeof(st));
    reply->set_return_code(0);
    return grpc::Status::OK;
  }

  grpc::Status Open(grpc::ServerContext*, const rdmafs::OpenRequest* req,
                    rdmafs::OpenReply* rep) override {
    std::lock_guard<std::mutex> lock(global_mutex_);

    int fd = open((base_ + req->path()).c_str(), req->flags());
    if (fd == -1)
      return grpc::Status(grpc::StatusCode::UNKNOWN, strerror(errno));

    rep->set_fh(static_cast<uint64_t>(fd));
    return grpc::Status::OK;
  }

  grpc::Status Readdir(grpc::ServerContext*, const rdmafs::Path* req, rdmafs::ReaddirReply* reply) {
      DIR* dp = opendir(full_path(req->path()).c_str());
      if (!dp) {
          reply->set_code(-errno);
          return grpc::Status::OK;
      }

      dirent* de;
      while ((de = readdir(dp))) {
          rdmafs::Dirent* entry = reply->add_entries();
          entry->set_name(de->d_name);
          entry->set_ino(de->d_ino);
          entry->set_mode(de->d_type << 12);
      }

      closedir(dp);
      reply->set_code(0);
      return grpc::Status::OK;
  }

  // …implement the rest
 private: 
  std::mutex global_mutex_;
  inline std::string full_path(std::string& path){
    std::string result = base_;
    if(path[0] == '/'){
      result += std::string{path};
    }else{
      result += ("/" + std::string{path});
    }
    return result;
  }
  std::string base_;
};

int main() {
  const std::string addr{"0.0.0.0:50051"};
  BackuperServiceImpl svc("/home/alex/workspace/fs_backup_1");

  grpc::ServerBuilder builder;
  builder.AddListeningPort(addr, grpc::InsecureServerCredentials());
  builder.RegisterService(&svc);
  std::unique_ptr<grpc::Server> server = builder.BuildAndStart();
  std::cout << "Backuper listening on " << addr << '\n';
  server->Wait();
}
