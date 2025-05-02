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

int main(int argc, char* argv[]) {
    fuse_operations oper {};
    oper.init = rdmafs_init;
    oper.getattr = rdmafs_getattr;
    oper.readdir = rdmafs_readdir;
    oper.open = rdmafs_open;
    oper.create = rdmafs_create;
    oper.read = rdmafs_read;
    oper.write = rdmafs_write;
    return fuse_main(argc, argv, &oper, nullptr);
}