/**
 * g++ -Wall -std=c++20 Rdmafs.cpp `pkg-config fuse3 --cflags --libs` -o rdmafs
 * We implement the FUSE API for now.
 */
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

#define USE_

void* rdmafs_init(fuse_conn_info*, fuse_config*) {
    /**
     * cfg->direct_io = 1;
     * cfg->parallel_direct_writes = 1;
     */
    return nullptr;
}

int rdmafs_getattr(const char* path, struct stat* stbuf, fuse_file_info*) {
    if(lstat(path, stbuf)==-1) return -errno;
    return 0;
}

int rdmafs_access(const char *path, int mask) {
	if (access(path, mask) == -1) return -errno;
	return 0;
}

int rdmafs_readlink(const char *path, char *buf, size_t size) {
    int res;
	if ( (res = readlink(path, buf, size - 1)) == -1) return -errno;
	buf[res] = '\0';
	return 0;
}

int rdmafs_readdir(const char *path, void *buf, fuse_fill_dir_t filler,
    off_t offset, struct fuse_file_info *fi,
    enum fuse_readdir_flags flags) {
    
    DIR *dp;
    dirent *de;

    dp = opendir(path);
    if (!dp) return -errno;

    while ((de = readdir(dp))) {
        struct stat st;
        memset(&st, 0, sizeof(st));
        st.st_ino = de->d_ino;
        st.st_mode = de->d_type << 12;
        if (filler(buf, de->d_name, &st, 0, FUSE_FILL_DIR_DEFAULTS)) break;
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
#ifdef __FreeBSD__
	} else if (S_ISSOCK(mode)) {
		struct sockaddr_un su;
		int fd;

		if (strlen(path) >= sizeof(su.sun_path)) {
			errno = ENAMETOOLONG;
			return -1;
		}
		fd = socket(AF_UNIX, SOCK_STREAM, 0);
		if (fd >= 0) {
			/*
			 * We must bind the socket to the underlying file
			 * system to create the socket file, even though
			 * we'll never listen on this socket.
			 */
			su.sun_family = AF_UNIX;
			strncpy(su.sun_path, path, sizeof(su.sun_path));
			res = bindat(dirfd, fd, (struct sockaddr*)&su,
				sizeof(su));
			if (res == 0)
				close(fd);
		} else {
			res = -1;
		}
#endif
	} else {
		res = mknodat(dirfd, path, mode, rdev);
	}

	return res;
}

int rdmafs_mknod(const char *path, mode_t mode, dev_t rdev) {
	int res;

	res = mknod_wrapper(AT_FDCWD, path, NULL, mode, rdev);
	if (res == -1)
		return -errno;

	return 0;
}

int rdmafs_open(const char *path, struct fuse_file_info *fi) {
    int64_t fh;
	if ( (fh = open(path, fi->flags)) == -1) return -errno;

    /* Enable direct_io when open has flags O_DIRECT to enjoy the feature
    parallel_direct_writes (i.e., to get a shared lock, not exclusive lock,
    for writes to the same file). */
	if (fi->flags & O_DIRECT) {
		fi->direct_io = 1;
		fi->parallel_direct_writes = 1;
	}

	fi->fh = fh;
	return 0;
}

int rdmafs_create(const char *path, mode_t mode, struct fuse_file_info *fi) {
    int64_t fh;
	if ( (fh = open(path, fi->flags, mode)) == -1) return -errno;

    /* Enable direct_io when open has flags O_DIRECT to enjoy the feature
    parallel_direct_writes (i.e., to get a shared lock, not exclusive lock,
    for writes to the same file). */
	if (fi->flags & O_DIRECT) {
		fi->direct_io = 1;
		fi->parallel_direct_writes = 1;
	}

	fi->fh = fh;
	return 0;
}

int rdmafs_read(const char *path, char *buf, size_t size, off_t offset,
    struct fuse_file_info *fi) {
    int fd;
    if(!fi) fd = open(path, O_RDONLY);
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
    if(!fi) fd = open(path, O_WRONLY);
    else fd = fi->fh;

    if (fd == -1)
    return -errno;

    int res = pwrite(fd, buf, size, offset);
    if (res == -1) res = -errno;

    if(!fi) close(fd);
    return res;
}

int rdmafs_mkdir(const char *path, mode_t mode) {
	if (mkdir(path, mode) == -1) return -errno;
	return 0;
}

int rdmafs_unlink(const char *path) {
	if (unlink(path) == -1) return -errno;
	return 0;
}

int rdmafs_rmdir(const char *path) {
	if (rmdir(path) == -1) return -errno;
	return 0;
}

int rdmafs_symlink(const char *from, const char *to) {
	if (symlink(from, to) == -1) return -errno;
	return 0;
}

int rdmafs_rename(const char *from, const char *to, unsigned int flags) {
	if (flags) return -EINVAL;
	if (rename(from, to) == -1) return -errno;
	return 0;
}

int rdmafs_link(const char *from, const char *to) {
	if (link(from, to) == -1) return -errno;
	return 0;
}

int rdmafs_chmod(const char *path, mode_t mode,
		     struct fuse_file_info*) {
	if (chmod(path, mode) == -1) return -errno;
	return 0;
}

int rdmafs_chown(const char *path, uid_t uid, gid_t gid,
		     struct fuse_file_info*) {
	if (lchown(path, uid, gid) == -1) return -errno;
	return 0;
}

int rdmafs_truncate(const char *path, off_t size,
			struct fuse_file_info *fi) {
	int res;
	if (fi) res = ftruncate(fi->fh, size);
	else res = truncate(path, size);

	if (res == -1) return -errno;
	return 0;
}

int rdmafs_release(const char *path, struct fuse_file_info *fi) {
	close(fi->fh);
	return 0;
}

int rdmafs_statfs(const char *path, struct statvfs *stbuf) {
	if (statvfs(path, stbuf) == -1) return -errno;
	return 0;
}

int main(int argc, char* argv[]) {
    fuse_operations oper {};
    oper.init = rdmafs_init;
    oper.getattr = rdmafs_getattr;
    oper.access = rdmafs_access;
    oper.readlink = rdmafs_readlink;
    oper.readdir = rdmafs_readdir;
    oper.open = rdmafs_open;
    oper.create = rdmafs_create;
    oper.read = rdmafs_read;
    oper.write = rdmafs_write;
    oper.mkdir = rdmafs_mkdir;
    oper.unlink = rdmafs_unlink;
    oper.rmdir = rdmafs_rmdir;
    oper.symlink = rdmafs_symlink;
    oper.rename = rdmafs_rename;
    oper.link = rdmafs_link;
    oper.chmod = rdmafs_chmod;
    oper.chown = rdmafs_chown;
    oper.truncate = rdmafs_truncate;
    oper.statfs = rdmafs_statfs;
    oper.release = rdmafs_release;
    return fuse_main(argc, argv, &oper, nullptr);
}