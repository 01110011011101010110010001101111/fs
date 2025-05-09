#include <grpcpp/grpcpp.h>
#include <google/protobuf/timestamp.pb.h>
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

  grpc::Status Hello(grpc::ServerContext*, const rdmafs::Hellorequest* req,
                     rdmafs::Helloreply* reply) override {
    reply->set_message("Hello, " + req->name() + "!");
    return grpc::Status::OK;
  }

  grpc::Status Utimens(grpc::ServerContext*, const rdmafs::Utimensrequest* req,
                       rdmafs::Messagestatus* reply) override {
    std::lock_guard<std::mutex> lock(global_mutex_);
    struct timespec ts[2];
    ts[0].tv_sec = req->access_time().seconds();
    ts[0].tv_nsec = req->access_time().nanos();
    ts[1].tv_sec = req->modify_time().seconds();
    ts[1].tv_nsec = req->modify_time().nanos();
    if(utimensat(0, full_path(req->path()).c_str(), ts, AT_SYMLINK_NOFOLLOW) == -1) {
      reply->set_return_code(-errno);
      return grpc::Status::OK;
    }
    reply->set_return_code(0);
    return grpc::Status::OK;
  }

  grpc::Status Getattr(grpc::ServerContext*, const rdmafs::Path* req,
                       rdmafs::Attr* reply) override {
    std::lock_guard<std::mutex> lock(global_mutex_);
    struct stat st;
    if(lstat(full_path(req->path()).c_str(), &st) == -1) {
      reply->set_return_code(-errno);
      return grpc::Status::OK;
    }
    reply->set_stat(reinterpret_cast<char*>(&st), sizeof(st));
    reply->set_return_code(0);
    return grpc::Status::OK;
  }

  grpc::Status Access(grpc::ServerContext*, const rdmafs::Accessrequest* req,
                      rdmafs::Messagestatus* reply) override {
    std::lock_guard<std::mutex> lock(global_mutex_);
    if(access(full_path(req->path()).c_str(), req->mask()) == -1) {
      reply->set_return_code(-errno);
      return grpc::Status::OK;
    }
    reply->set_return_code(0);
    return grpc::Status::OK;
  }

  grpc::Status Readlink(grpc::ServerContext*, const rdmafs::Readlinkrequest* req,
                        rdmafs::Readlinkreply* reply) override {
    std::lock_guard<std::mutex> lock(global_mutex_);
    size_t bufsize = static_cast<size_t>(req->size());
    if (bufsize==0 || bufsize>PATH_MAX) {
      reply->set_return_code(-EINVAL);
      return grpc::Status::OK;
    }
    std::vector<char> buf(bufsize);
    int res = readlink(full_path(req->path()).c_str(), buf.data(), bufsize-1);
    if (res == -1) {
      reply->set_return_code(-errno);
      return grpc::Status::OK;
    }
    buf[std::min(static_cast<size_t>(res), bufsize-1)] = '\0';
    reply->set_data(std::string{buf.data()});
    reply->set_return_code(0);
    return grpc::Status::OK;
  }

  grpc::Status Readdir(grpc::ServerContext*, const rdmafs::Path* req, rdmafs::Readdirreply* reply) {
    std::lock_guard<std::mutex> lock(global_mutex_);
    DIR* dp = opendir(full_path(req->path()).c_str());
    if (!dp) {
      reply->set_return_code(-errno);
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
    reply->set_return_code(0);
    return grpc::Status::OK;
  }

  grpc::Status Mknod(grpc::ServerContext*, const rdmafs::Mknodrequest* req,
                     rdmafs::Messagestatus* reply) override {
    std::lock_guard<std::mutex> lock(global_mutex_);
    if (mknod_wrapper(AT_FDCWD, full_path(req->path()).c_str(), nullptr, req->mode(), req->rdev()) == -1) {
      reply->set_return_code(-errno);
      return grpc::Status::OK;
    }
    reply->set_return_code(0);
    return grpc::Status::OK;
  }
  

  grpc::Status Open(grpc::ServerContext*, const rdmafs::Openrequest* req,
                    rdmafs::Openreply* reply) override {
    std::lock_guard<std::mutex> lock(global_mutex_);
    int64_t fh;
    if ( (fh = open(full_path(req->path()).c_str(), req->flags())) == -1) {
      reply->set_return_code(-errno);
      return grpc::Status::OK;
    }
    reply->set_fh(static_cast<uint64_t>(fh));
    reply->set_return_code(0);
    return grpc::Status::OK;
  }

  grpc::Status Create(grpc::ServerContext*, const rdmafs::Createrequest* req,
                      rdmafs::Openreply* reply) override {
    std::lock_guard<std::mutex> lock(global_mutex_);
    int64_t fh;
    if ( (fh = open(full_path(req->path()).c_str(), req->flags(), req->mode())) == -1) {
      reply->set_return_code(-errno);
      return grpc::Status::OK;
    }
    reply->set_fh(static_cast<uint64_t>(fh));
    reply->set_return_code(0);
    return grpc::Status::OK;
  }

  grpc::Status Read(grpc::ServerContext*, const rdmafs::Readrequest* req,
                    rdmafs::Readreply* reply) override {
    std::lock_guard<std::mutex> lock(global_mutex_);
    ssize_t fd;
    if (req->has_fh()) fd = req->fh();
    else fd = open(full_path(req->path()).c_str(), O_RDONLY);

    if (fd == -1) {
        reply->set_return_code(-errno);
        return grpc::Status::OK;
    }

    std::string buffer;
    buffer.resize(req->size());

    int res = pread(fd, buffer.data(), req->size(), req->offset());

    if (!req->has_fh()) close(fd);

    if (res == -1) {
        reply->set_return_code(-errno);
    } else {
        reply->set_return_code(res);
        reply->set_data(buffer.data(), res);
    }

    return grpc::Status::OK;
  }

  grpc::Status Write(grpc::ServerContext* context,
                     const rdmafs::Writerequest* req,
                     rdmafs::Writereply* reply) override {
    const std::string& path = req->path();
    const std::string& data = req->data();
    off_t offset = req->offset();

    ssize_t fd;
    if (req->has_fh()) {
      fd = req->fh();
    } else {
      fd = open(path.c_str(), O_WRONLY);
    }

    if (fd == -1) {
      reply->set_return_code(-errno);
      return grpc::Status::OK;
    }

    ssize_t written = pwrite(fd, data.data(), data.size(), offset);
    if (written == -1) {
      reply->set_return_code(-errno);
    } else {
      reply->set_return_code(written);  // return number of bytes written
    }

    if (!req->has_fh()) close(fd);
    return grpc::Status::OK;
  }

  grpc::Status Mkdir(grpc::ServerContext* context,
                     const rdmafs::Mkdirrequest* req,
                     rdmafs::Messagestatus* reply) override {
    std::lock_guard<std::mutex> lock(global_mutex_);
    if (mkdir(full_path(req->path()).c_str(), req->mode()) == -1) {
      reply->set_return_code(-errno);
      return grpc::Status::OK;
    }
    reply->set_return_code(0);
    return grpc::Status::OK;
  }

  grpc::Status Unlink(grpc::ServerContext* context,
                      const rdmafs::Path* req,
                      rdmafs::Messagestatus* reply) override {
    std::lock_guard<std::mutex> lock(global_mutex_);
    if (unlink(full_path(req->path()).c_str()) == -1) {
      reply->set_return_code(-errno);
      return grpc::Status::OK;
    }
    reply->set_return_code(0);
    return grpc::Status::OK;
  }

  grpc::Status Rmdir(grpc::ServerContext* context,
                     const rdmafs::Path* req,
                     rdmafs::Messagestatus* reply) override {
    std::lock_guard<std::mutex> lock(global_mutex_);
    if (rmdir(full_path(req->path()).c_str()) == -1) {
      reply->set_return_code(-errno);
      return grpc::Status::OK;
    }
    reply->set_return_code(0);
    return grpc::Status::OK;
  }

  grpc::Status Symlink(grpc::ServerContext* context,
                       const rdmafs::Symlinkrequest* req,
                       rdmafs::Messagestatus* reply) override {
    std::lock_guard<std::mutex> lock(global_mutex_);
    if (symlink(full_path(req->from()).c_str(), full_path(req->to()).c_str()) == -1) {
      reply->set_return_code(-errno);
      return grpc::Status::OK;
    }
    reply->set_return_code(0);
    return grpc::Status::OK;
  }

  grpc::Status Rename(grpc::ServerContext* context,
                      const rdmafs::Symlinkrequest* req,
                      rdmafs::Messagestatus* reply) override {
    std::lock_guard<std::mutex> lock(global_mutex_);
    if (rename(full_path(req->from()).c_str(), full_path(req->to()).c_str()) == -1) {
      reply->set_return_code(-errno);
      return grpc::Status::OK;
    }
    reply->set_return_code(0);
    return grpc::Status::OK;
  }

  grpc::Status Link(grpc::ServerContext* context,
                    const rdmafs::Symlinkrequest* req,
                    rdmafs::Messagestatus* reply) override {
    std::lock_guard<std::mutex> lock(global_mutex_);
    if (link(full_path(req->from()).c_str(), full_path(req->to()).c_str()) == -1) {
      reply->set_return_code(-errno);
      return grpc::Status::OK;
    }
    reply->set_return_code(0);
    return grpc::Status::OK;
  }

  grpc::Status Chmod(grpc::ServerContext* context,
                     const rdmafs::Chmodrequest* req,
                     rdmafs::Messagestatus* reply) override {
    std::lock_guard<std::mutex> lock(global_mutex_);
    if (chmod(full_path(req->path()).c_str(), req->mode()) == -1) {
      reply->set_return_code(-errno);
      return grpc::Status::OK;
    }
    reply->set_return_code(0);
    return grpc::Status::OK;
  }

  grpc::Status Chown(grpc::ServerContext* context,
                     const rdmafs::Chownrequest* req,
                     rdmafs::Messagestatus* reply) override {
    std::lock_guard<std::mutex> lock(global_mutex_);
    if (lchown(full_path(req->path()).c_str(), req->uid(), req->gid()) == -1) {
      reply->set_return_code(-errno);
      return grpc::Status::OK;
    }
    reply->set_return_code(0);
    return grpc::Status::OK;
  }

  grpc::Status Truncate(grpc::ServerContext* context,
                        const rdmafs::Truncaterequest* req,
                        rdmafs::Messagestatus* reply) override {
    std::lock_guard<std::mutex> lock(global_mutex_);
    if (req->has_fh()) {
      if (ftruncate(req->fh(), req->size()) == -1) {
        reply->set_return_code(-errno);
        return grpc::Status::OK;
      }
    } else {
      if (truncate(full_path(req->path()).c_str(), req->size()) == -1) {
        reply->set_return_code(-errno);
        return grpc::Status::OK;
      }
    }
    reply->set_return_code(0);
    return grpc::Status::OK;
  }

  grpc::Status Release(grpc::ServerContext* context,
                       const rdmafs::Releaserequest* req,
                       rdmafs::Messagestatus* reply) override {
    std::lock_guard<std::mutex> lock(global_mutex_);
    close(req->fh());
    reply->set_return_code(0);
    return grpc::Status::OK;
  }

  grpc::Status Statfs(grpc::ServerContext* ctx,
                    const rdmafs::Statfsrequest* req,
                    rdmafs::Statfsreply* rep) override {
    struct statvfs buf;
    if (statvfs(full_path(req->path()).c_str(), &buf) == -1) {
      rep->set_return_code(-errno);
      return grpc::Status::OK;
    }

    rep->set_return_code(0);
    rep->set_f_bsize(buf.f_bsize);
    rep->set_f_frsize(buf.f_frsize);
    rep->set_f_blocks(buf.f_blocks);
    rep->set_f_bfree(buf.f_bfree);
    rep->set_f_bavail(buf.f_bavail);
    rep->set_f_files(buf.f_files);
    rep->set_f_ffree(buf.f_ffree);
    rep->set_f_favail(buf.f_favail);
    rep->set_f_fsid(buf.f_fsid);
    rep->set_f_flag(buf.f_flag);
    rep->set_f_namemax(buf.f_namemax);
    return grpc::Status::OK;
  }

 private: 
  std::mutex global_mutex_;
  inline std::string full_path(const std::string& path){
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

int main(int argc, char* argv[]) {
  std::string base_path{"/home/alex/workspace/fs_backup_1"};
  if (argc > 1) {
		base_path = argv[1];	
	}
  const std::string addr{"0.0.0.0:50051"};
  BackuperServiceImpl svc(base_path);

  grpc::ServerBuilder builder;
  builder.AddListeningPort(addr, grpc::InsecureServerCredentials());
  builder.RegisterService(&svc);
  std::unique_ptr<grpc::Server> server = builder.BuildAndStart();
  std::cout << "Server listening on " << addr << '\n';
  server->Wait();
}
