/**
 * g++ -Wall -std=c++17 Rdmafs.cpp `pkg-config fuse3 --cflags --libs` -I/home/alex/workspace/local_grpc/include -Ibuild -L/home/alex/workspace/local_grpc/lib -lgrpc++ -lprotobuf -o rdmafs_test
 * We implement the FUSE API for now.
 */
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

#ifdef HAVE_SETXATTR
#include <sys/xattr.h>
#endif

// const std::string base_path = "/home/alexhu/orcd/c7/pool/test1";
std::string base_path = "/home/alex/workspace/fs_mount1";
std::unique_ptr<rdmafs::Backuper::Stub> g_stub;

inline std::string full_path(const char*path){
	std::string result = base_path;
	if(path[0] == '/'){
		result += std::string{path};
	}else{
		result += ("/" + std::string{path});
	}
	return result;
}

int rdmafs_utimens(const char *path, const struct timespec ts[2],
		       struct fuse_file_info*) {
	/* don't use utime/utimes since they follow symlinks */
	if (utimensat(0, full_path(path).c_str(), ts, AT_SYMLINK_NOFOLLOW) == -1) return -errno;
	return 0;
}

void* rdmafs_init(fuse_conn_info*, fuse_config*) {
    /**
     * cfg->direct_io = 1;
     * cfg->parallel_direct_writes = 1;
     */
    return nullptr;
}

int rdmafs_getattr(const char* path, struct stat* stbuf, fuse_file_info*) {
    if(lstat(full_path(path).c_str(), stbuf)==-1) return -errno;
    return 0;
}

int rdmafs_access(const char *path, int mask) {
	if (access(full_path(path).c_str(), mask) == -1) return -errno;
	return 0;
}

int rdmafs_readlink(const char *path, char *buf, size_t size) {
    int res;
	if ( (res = readlink(full_path(path).c_str(), buf, size - 1)) == -1) return -errno;
	buf[res] = '\0';
	return 0;
}

int rdmafs_readdir(const char *path, void *buf, fuse_fill_dir_t filler,
    off_t offset, struct fuse_file_info *fi,
    enum fuse_readdir_flags flags) {
    DIR* dp = opendir(full_path(path).c_str());
    if (!dp) return -errno;

	dirent *de;
    while ((de = readdir(dp))) {
        struct stat st;
        memset(&st, 0, sizeof(st));
        st.st_ino = de->d_ino;
        st.st_mode = de->d_type << 12;
        // if (filler(buf, de->d_name, &st, 0, FUSE_FILL_DIR_DEFAULTS)) break;
		if (filler(buf, de->d_name, &st, 0, FUSE_FILL_DIR_PLUS)) break;
    }
    closedir(dp);
    return 0;
}

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

int rdmafs_mknod(const char *path, mode_t mode, dev_t rdev) {
	int res;

	res = mknod_wrapper(AT_FDCWD, full_path(path).c_str(), NULL, mode, rdev);
	if (res == -1)
		return -errno;

	return 0;
}

int rdmafs_open(const char *path, struct fuse_file_info *fi) {
    int64_t fh;
	if ( (fh = open(full_path(path).c_str(), fi->flags)) == -1) return -errno;

    /* Enable direct_io when open has flags O_DIRECT to enjoy the feature
    parallel_direct_writes (i.e., to get a shared lock, not exclusive lock,
    for writes to the same file). */
	if (fi->flags & O_DIRECT) {
		fi->direct_io = 1;
		// fi->parallel_direct_writes = 1;
	}

	fi->fh = fh;
	return 0;
}

int rdmafs_create(const char *path, mode_t mode, struct fuse_file_info *fi) {
    int64_t fh;
	if ( (fh = open(full_path(path).c_str(), fi->flags, mode)) == -1) return -errno;

    /* Enable direct_io when open has flags O_DIRECT to enjoy the feature
    parallel_direct_writes (i.e., to get a shared lock, not exclusive lock,
    for writes to the same file). */
	if (fi->flags & O_DIRECT) {
		fi->direct_io = 1;
		// fi->parallel_direct_writes = 1;
	}

	fi->fh = fh;
	return 0;
}

int rdmafs_read(const char *path, char *buf, size_t size, off_t offset,
    struct fuse_file_info *fi) {
    int fd;
    if(!fi) fd = open(full_path(path).c_str(), O_RDONLY);
    else fd = fi->fh;

    if (fd == -1) return -errno;

    int res = pread(fd, buf, size, offset);
    if (res == -1) res = -errno;

    if(!fi) close(fd);
    return res;
}

int rdmafs_write(const char *path, const char *buf, size_t size,
    off_t offset, struct fuse_file_info *fi) {
    int fd;
    if(!fi) fd = open(full_path(path).c_str(), O_WRONLY);
    else fd = fi->fh;

    if (fd == -1)
    return -errno;

    int res = pwrite(fd, buf, size, offset);
    if (res == -1) res = -errno;

    if(!fi) close(fd);
    return res;
}

int rdmafs_mkdir(const char *path, mode_t mode) {
	if (mkdir(full_path(path).c_str(), mode) == -1) return -errno;
	return 0;
}

int rdmafs_unlink(const char *path) {
	if (unlink(full_path(path).c_str()) == -1) return -errno;
	return 0;
}

int rdmafs_rmdir(const char *path) {
	if (rmdir(full_path(path).c_str()) == -1) return -errno;
	return 0;
}

int rdmafs_symlink(const char *from, const char *to) {
	if (symlink(full_path(from).c_str(), full_path(to).c_str()) == -1) return -errno;
	return 0;
}

int rdmafs_rename(const char *from, const char *to, unsigned int flags) {
	if (flags) return -EINVAL;
	if (rename(full_path(from).c_str(), full_path(to).c_str()) == -1) return -errno;
	return 0;
}

int rdmafs_link(const char *from, const char *to) {
	if (link(full_path(from).c_str(), full_path(to).c_str()) == -1) return -errno;
	return 0;
}

int rdmafs_chmod(const char *path, mode_t mode,
		     struct fuse_file_info*) {
	if (chmod(full_path(path).c_str(), mode) == -1) return -errno;
	return 0;
}

int rdmafs_chown(const char *path, uid_t uid, gid_t gid,
		     struct fuse_file_info*) {
	if (lchown(full_path(path).c_str(), uid, gid) == -1) return -errno;
	return 0;
}

int rdmafs_truncate(const char *path, off_t size,
			struct fuse_file_info *fi) {
	int res;
	if (fi) res = ftruncate(fi->fh, size);
	else res = truncate(full_path(path).c_str(), size);

	if (res == -1) return -errno;
	return 0;
}

int rdmafs_release(const char *path, struct fuse_file_info *fi) {
	close(fi->fh);
	return 0;
}

int rdmafs_statfs(const char *path, struct statvfs *stbuf) {
	if (statvfs(full_path(path).c_str(), stbuf) == -1) return -errno;
	return 0;
}

int rdmafs_backup_utimens(const char *path, const struct timespec ts[2],
		       struct fuse_file_info*) {
	rdmafs::Utimensrequest req;
	rdmafs::Messagestatus reply;
	grpc::ClientContext ctx;
	req.set_path(path);
	req.mutable_access_time()->set_seconds(ts[0].tv_sec);
	req.mutable_access_time()->set_nanos(ts[0].tv_nsec);
	req.mutable_modify_time()->set_seconds(ts[1].tv_sec);
	req.mutable_modify_time()->set_nanos(ts[1].tv_nsec);
	auto status = g_stub->Utimens(&ctx, req, &reply);
	if (!status.ok()) {
		return -EIO;
	} else {
		return reply.return_code();
	}
}

void* rdmafs_backup_init(fuse_conn_info*, fuse_config*) {
    /**
     * cfg->direct_io = 1;
     * cfg->parallel_direct_writes = 1;
     */	
    return nullptr;
}

int rdmafs_backup_getattr(const char* path, struct stat* stbuf, fuse_file_info*) {
    // if(lstat(full_path(path).c_str(), stbuf)==-1) return -errno;
	rdmafs::Path req;
	rdmafs::Attr attr;
	grpc::ClientContext ctx;
	req.set_path(path);
	auto status = g_stub->Getattr(&ctx, req, &attr);
	if (!status.ok()) {
		return -EIO;
	} else {
		if (attr.return_code() == 0) {
			memcpy(stbuf, attr.stat().c_str(), sizeof(struct stat));
			return 0;
		} else {
			return attr.return_code();
		}
	}
}

int rdmafs_backup_access(const char *path, int mask) {
	rdmafs::Accessrequest req;
	rdmafs::Messagestatus reply;
	grpc::ClientContext ctx;
	req.set_path(path);
	req.set_mask(mask);
	auto status = g_stub->Access(&ctx, req, &reply);
	if (!status.ok()) {
		return -EIO;
	} else {
		return reply.return_code();
	}
}

int rdmafs_backup_readlink(const char *path, char *buf, size_t size) {
	rdmafs::Readlinkrequest req;
	rdmafs::Readlinkreply reply;
	grpc::ClientContext ctx;
	req.set_path(path);
	req.set_size(static_cast<int32_t>(size));
	auto status = g_stub->Readlink(&ctx, req, &reply);
	if (!status.ok()) {
		return -EIO;
	} else {
		if (reply.return_code() == 0) {
			const std::string& data = reply.data();
			size_t copy_len = std::min(size-1, data.size());
			memcpy(buf, data.c_str(), copy_len);
			buf[copy_len] = '\0';
			return 0;
		} else {
			return reply.return_code();
		}
	}
}

int rdmafs_backup_readdir(const char *path, void *buf, fuse_fill_dir_t filler,
    off_t offset, struct fuse_file_info *fi,
    enum fuse_readdir_flags flags) {
	// offset ignored
	rdmafs::Path req;
	req.set_path(path);
	rdmafs::Readdirreply reply;
	grpc::ClientContext ctx;
	auto status = g_stub->Readdir(&ctx, req, &reply);
	if (!status.ok()) {
		return -EIO;
	}
	if (reply.return_code() != 0) return reply.return_code();
	for (const auto& entry : reply.entries()) {
		struct stat st;
		memset(&st, 0, sizeof(st));
		st.st_ino = entry.ino();
		st.st_mode = entry.mode();
		if (filler(buf, entry.name().c_str(), &st, 0, FUSE_FILL_DIR_PLUS)) break;
	}
	return 0;
}

int rdmafs_backup_mknod(const char *path, mode_t mode, dev_t rdev) {
	rdmafs::Mknodrequest req;
	rdmafs::Messagestatus reply;
	grpc::ClientContext ctx;
	req.set_path(path);
	req.set_mode(mode);
	req.set_rdev(rdev);
	auto status = g_stub->Mknod(&ctx, req, &reply);
	if (!status.ok()) {
		return -EIO;
	} else {
		return reply.return_code();
	}
}

int rdmafs_backup_open(const char *path, struct fuse_file_info *fi) {
	rdmafs::Openrequest req;
	rdmafs::Openreply reply;
	grpc::ClientContext ctx;
	req.set_path(path);
	req.set_flags(fi->flags);
	auto status = g_stub->Open(&ctx, req, &reply);
	if (!status.ok()) {
		return -EIO;
	} else {
		if (reply.return_code() == 0) {
			fi->fh = reply.fh();
			if (fi->flags & O_DIRECT) {
				fi->direct_io = 1;
			}
			return 0;
		} else {
			return reply.return_code();
		}
	}
}

int rdmafs_backup_create(const char *path, mode_t mode, struct fuse_file_info *fi) {
	rdmafs::Createrequest req;
	rdmafs::Openreply reply;
	grpc::ClientContext ctx;
	req.set_path(path);
	req.set_flags(fi->flags);
	req.set_mode(mode);
	auto status = g_stub->Create(&ctx, req, &reply);
	if (!status.ok()) {
		return -EIO;
	} else {
		if (reply.return_code() == 0) {
			fi->fh = reply.fh();
			if (fi->flags & O_DIRECT) {
				fi->direct_io = 1;
			}
			return 0;
		} else {
			return reply.return_code();
		}
	}
}

int rdmafs_backup_read(const char *path, char *buf, size_t size, off_t offset,
    struct fuse_file_info *fi) {
    rdmafs::Readrequest req;
    req.set_path(path);
    req.set_offset(offset);
    req.set_size(size);
    if (fi) req.set_fh(fi->fh);
    rdmafs::Readreply reply;
    grpc::ClientContext ctx;
    auto status = g_stub->Read(&ctx, req, &reply);
    if (!status.ok()) return -EIO;
    int ret = reply.return_code();
    if (ret < 0) return ret;
    memcpy(buf, reply.data().data(), ret);
    return ret;
}

int rdmafs_backup_write(const char *path, const char *buf, size_t size,
    off_t offset, struct fuse_file_info *fi) {
	rdmafs::Writerequest req;
	rdmafs::Writereply rep;
	grpc::ClientContext ctx;
	req.set_path(path);
	req.set_data(buf, size);
	req.set_offset(offset);
	req.set_size(size);
	auto status = g_stub->Write(&ctx, req, &rep);
	if (!status.ok()) return -EIO;
	return rep.return_code();
}

int rdmafs_backup_mkdir(const char *path, mode_t mode) {
	rdmafs::Mkdirrequest req;
	rdmafs::Messagestatus reply;
	grpc::ClientContext ctx;
	req.set_path(path);
	req.set_mode(mode);
	auto status = g_stub->Mkdir(&ctx, req, &reply);
	if (!status.ok()) return -EIO;
	return reply.return_code();
}

int rdmafs_backup_unlink(const char *path) {
	rdmafs::Path req;
	rdmafs::Messagestatus reply;
	grpc::ClientContext ctx;
	req.set_path(path);
	auto status = g_stub->Unlink(&ctx, req, &reply);
	if (!status.ok()) return -EIO;
	return reply.return_code();
}

int rdmafs_backup_rmdir(const char *path) {
	rdmafs::Path req;
	rdmafs::Messagestatus reply;
	grpc::ClientContext ctx;
	req.set_path(path);
	auto status = g_stub->Rmdir(&ctx, req, &reply);
	if (!status.ok()) return -EIO;
	return reply.return_code();
}

int rdmafs_backup_symlink(const char *from, const char *to) {
	rdmafs::Symlinkrequest req;
	rdmafs::Messagestatus reply;
	grpc::ClientContext ctx;
	req.set_from(from);
	req.set_to(to);
	auto status = g_stub->Symlink(&ctx, req, &reply);
	if (!status.ok()) return -EIO;
	return reply.return_code();
}

int rdmafs_backup_rename(const char *from, const char *to, unsigned int flags) {
	if (flags) return -EINVAL;
	rdmafs::Symlinkrequest req;
	rdmafs::Messagestatus reply;
	grpc::ClientContext ctx;
	req.set_from(from);
	req.set_to(to);
	auto status = g_stub->Rename(&ctx, req, &reply);
	if (!status.ok()) return -EIO;
	return reply.return_code();
}

int rdmafs_backup_link(const char *from, const char *to) {
	rdmafs::Symlinkrequest req;
	rdmafs::Messagestatus reply;
	grpc::ClientContext ctx;
	req.set_from(from);
	req.set_to(to);
	auto status = g_stub->Link(&ctx, req, &reply);
	if (!status.ok()) return -EIO;
	return reply.return_code();
}

int rdmafs_backup_chmod(const char *path, mode_t mode,
		     struct fuse_file_info*) {
	rdmafs::Chmodrequest req;
	rdmafs::Messagestatus reply;
	grpc::ClientContext ctx;
	req.set_path(path);
	req.set_mode(mode);
	auto status = g_stub->Chmod(&ctx, req, &reply);
	if (!status.ok()) return -EIO;
	return reply.return_code();
}

int rdmafs_backup_chown(const char *path, uid_t uid, gid_t gid,
		     struct fuse_file_info*) {
	rdmafs::Chownrequest req;
	rdmafs::Messagestatus reply;
	grpc::ClientContext ctx;
	req.set_path(path);
	req.set_uid(uid);
	req.set_gid(gid);
	auto status = g_stub->Chown(&ctx, req, &reply);
	if (!status.ok()) return -EIO;
	return reply.return_code();
}

int rdmafs_backup_truncate(const char *path, off_t size,
			struct fuse_file_info *fi) {
	rdmafs::Truncaterequest req;
	rdmafs::Messagestatus reply;
	grpc::ClientContext ctx;
	req.set_path(path);
	req.set_size(size);
	if (fi) req.set_fh(fi->fh);
	auto status = g_stub->Truncate(&ctx, req, &reply);
	if (!status.ok()) return -EIO;
	return reply.return_code();
}

int rdmafs_backup_release(const char *path, struct fuse_file_info *fi) {
	rdmafs::Releaserequest req;
	rdmafs::Messagestatus reply;
	grpc::ClientContext ctx;
	req.set_fh(fi->fh);
	auto status = g_stub->Release(&ctx, req, &reply);
	if (!status.ok()) return -EIO;
	return reply.return_code();
}

int rdmafs_backup_statfs(const char *path, struct statvfs *stbuf) {
	rdmafs::Statfsrequest req;
	rdmafs::Statfsreply reply;
	grpc::ClientContext ctx;
	req.set_path(path);
	auto status = g_stub->Statfs(&ctx, req, &reply);
	if (!status.ok()) return -EIO;
	if (reply.return_code() != 0) return reply.return_code();
	stbuf->f_bsize = reply.f_bsize();
	stbuf->f_frsize = reply.f_frsize();
	stbuf->f_blocks = reply.f_blocks();
	stbuf->f_bfree = reply.f_bfree();
	stbuf->f_bavail = reply.f_bavail();
	stbuf->f_files = reply.f_files();
	stbuf->f_ffree = reply.f_ffree();
	stbuf->f_favail = reply.f_favail();
	stbuf->f_fsid = reply.f_fsid();
	stbuf->f_flag = reply.f_flag();
	stbuf->f_namemax = reply.f_namemax();
	return 0;
}


int main(int argc, char* argv[]) {
	// used when running local FUSE
	// if (argc > 1) {
	// 	base_path = argv[1];
	// 	for (int i = 0; i < argc - 1; i++) {
	// 		argv[i] = argv[i + 1];
	// 	}
	// 	argc--;
	// }
	auto channel = grpc::CreateChannel("0.0.0.0:50051", grpc::InsecureChannelCredentials());
	g_stub = rdmafs::Backuper::NewStub(channel);

	rdmafs::Hellorequest req;
	rdmafs::Helloreply reply;
	req.set_name("client");
	grpc::ClientContext ctx;
	auto status = g_stub->Hello(&ctx, req, &reply);
	if (!status.ok()) {
		return EIO;
	}
	std::cout << "Server's reply: " << reply.message() << std::endl;

    fuse_operations oper {};
    // oper.init = rdmafs_init;
    // oper.getattr = rdmafs_getattr;
    // oper.access = rdmafs_access;
    // oper.readlink = rdmafs_readlink;
    // oper.readdir = rdmafs_readdir;
    // oper.open = rdmafs_open;
    // oper.create = rdmafs_create;
    // oper.read = rdmafs_read;
    // oper.write = rdmafs_write;
    // oper.mkdir = rdmafs_mkdir;
    // oper.unlink = rdmafs_unlink;
    // oper.rmdir = rdmafs_rmdir;
    // oper.symlink = rdmafs_symlink;
    // oper.rename = rdmafs_rename;
    // oper.link = rdmafs_link;
    // oper.chmod = rdmafs_chmod;
    // oper.chown = rdmafs_chown;
    // oper.truncate = rdmafs_truncate;
    // oper.statfs = rdmafs_statfs;
    // oper.release = rdmafs_release;
	// oper.utimens = rdmafs_utimens;
	oper.init = rdmafs_backup_init;
	oper.getattr = rdmafs_backup_getattr;
	oper.access = rdmafs_backup_access;
	oper.readlink = rdmafs_backup_readlink;
	oper.readdir = rdmafs_backup_readdir;
	oper.open = rdmafs_backup_open;
	oper.create = rdmafs_backup_create;
	oper.read = rdmafs_backup_read;
	oper.write = rdmafs_backup_write;
	oper.mkdir = rdmafs_backup_mkdir;
	oper.unlink = rdmafs_backup_unlink;
	oper.rmdir = rdmafs_backup_rmdir;
	oper.symlink = rdmafs_backup_symlink;
	oper.rename = rdmafs_backup_rename;
	oper.link = rdmafs_backup_link;
	oper.chmod = rdmafs_backup_chmod;
	oper.chown = rdmafs_backup_chown;
	oper.truncate = rdmafs_backup_truncate;
	oper.statfs = rdmafs_backup_statfs;
	oper.release = rdmafs_backup_release;
	oper.utimens = rdmafs_backup_utimens;
    return fuse_main(argc, argv, &oper, nullptr);
}