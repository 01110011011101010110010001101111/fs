// rdma_common.hpp
#pragma once
#include <infiniband/verbs.h>
#include <rdma/rdma_cma.h>
#include <arpa/inet.h>
#include <cassert>
#include <cstring>
#include <iostream>

inline void die(const char* reason) { perror(reason); exit(EXIT_FAILURE); }

struct RDMABuffer {
    ibv_mr* mr{};
    void*   addr{};
    size_t  size{};
    uint32_t rkey() const { return mr->rkey; }
};

inline RDMABuffer register_memory(ibv_pd* pd, size_t bytes,
                                  int access = IBV_ACCESS_LOCAL_WRITE |
                                               IBV_ACCESS_REMOTE_WRITE |
                                               IBV_ACCESS_REMOTE_READ) {
    void* buf = aligned_alloc(4096, bytes); // page size 4096
    if (!buf) die("alloc");
    memset(buf, 0, bytes);
    ibv_mr* mr = ibv_reg_mr(pd, buf, bytes, access);
    if (!mr) die("reg_mr");
    return {mr, buf, bytes};
}

// // https://github.com/ofiwg/librdmacm/blob/master/examples/rping.c
// int get_addr(char *dst, struct sockaddr *addr)
// {
// 	struct addrinfo *res;
// 	int ret;

// 	ret = getaddrinfo(dst, NULL, NULL, &res);
// 	if (ret) {
// 		printf("getaddrinfo failed (%s) - invalid hostname or IP address\n", gai_strerror(ret));
// 		return ret;
// 	}

// 	if (res->ai_family == PF_INET)
// 		memcpy(addr, res->ai_addr, sizeof(struct sockaddr_in));
// 	else if (res->ai_family == PF_INET6)
// 		memcpy(addr, res->ai_addr, sizeof(struct sockaddr_in6));
// 	else
// 		ret = -1;
	
// 	freeaddrinfo(res);
// 	return ret;
// }

