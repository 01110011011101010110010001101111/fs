#include "rdma_common.hpp"

int main(int argc, char** argv) {
    const char* port = (argc > 1) ? argv[1] : "7471";

    rdma_cm_id* listener{};
    rdma_event_channel* ec = rdma_create_event_channel();
    if (rdma_create_id(ec, &listener, nullptr, RDMA_PS_TCP)) die("create_id");
    sockaddr_in addr { .sin_family = AF_INET, .sin_port = htons(atoi(port)) };
    addr.sin_addr.s_addr = htonl(INADDR_ANY); // listen on all interfaces
    if (rdma_bind_addr(listener, reinterpret_cast<sockaddr*>(&addr))) die("bind");
    if (rdma_listen(listener, 1)) die("listen");
    std::cout << "Listening on " << port << "…\n";

    rdma_cm_event* event{};
    rdma_get_cm_event(ec, &event);              // RDMA_CM_EVENT_CONNECT_REQUEST
    rdma_cm_id* id = event->id;
    rdma_ack_cm_event(event);

    // 1. Create verbs objects on this connection
    // Do I need to add a channel?
    ibv_pd*  pd  = ibv_alloc_pd(id->verbs);
    ibv_cq*  cq  = ibv_create_cq(id->verbs, 16, nullptr, nullptr, 0);
    ibv_qp_init_attr qp_attr {};
    qp_attr.send_cq = qp_attr.recv_cq = cq;
    qp_attr.cap.max_send_wr  = qp_attr.cap.max_recv_wr  = 16; // 8?
    qp_attr.cap.max_send_sge = qp_attr.cap.max_recv_sge = 1; // 2?
    qp_attr.qp_type = IBV_QPT_RC;
    if(rdma_create_qp(id, pd, &qp_attr)) die("create_qp");

    // 2. Register a 4 KiB buffer and share its addr+rkey with the peer
    auto buf = register_memory(pd, 4096);
    std::string metadata = std::to_string(reinterpret_cast<uint64_t>(buf.addr)) +
                           ":" + std::to_string(buf.rkey());

    // 3. Wait until peer sends its metadata in a SEND
    ibv_recv_wr  rwr { .wr_id = 1 };
    ibv_sge      sge { .addr = reinterpret_cast<uint64_t>(buf.addr),
                       .length = 64, .lkey = buf.mr->lkey };
    rwr.sg_list = &sge; rwr.num_sge = 1;
    if(ibv_post_recv(id->qp, &rwr, nullptr)) die("post_recv");

    // 4. Accept the connection request
    rdma_conn_param cp{};
    if(rdma_accept(id, &cp)) die("accept");

    // 5. Wait until we've received peer's meta
    ibv_wc wc{};
    while (ibv_poll_cq(cq, 1, &wc) == 0) ;
    if (wc.status != IBV_WC_SUCCESS || wc.opcode != IBV_WC_RECV) die("meta RECV failed");
    std::cout << "Received peer info: " << static_cast<char*>(buf.addr) << "\n";

    uint64_t peer_addr; uint32_t peer_rkey;
    sscanf(static_cast<char*>(buf.addr), "%lu:%u", &peer_addr, &peer_rkey);
    

    // 6. Issue one-sided WRITE
    constexpr const char msg[] = "Hello via RDMA WRITE!";
    strcpy(static_cast<char*>(buf.addr), msg);
    
    ibv_sge wsge { .addr = reinterpret_cast<uint64_t>(buf.addr),
                   .length = sizeof(msg), .lkey = buf.mr->lkey };
    ibv_send_wr swr{}, *bad{};
    swr.wr_id = 2; swr.sg_list = &wsge; swr.num_sge = 1; swr.opcode = IBV_WR_RDMA_WRITE; swr.send_flags = IBV_SEND_SIGNALED; swr.wr.rdma = {.remote_addr = peer_addr, .rkey = peer_rkey};

    if(ibv_post_send(id->qp, &swr, &bad)) die("post_send");
    while (ibv_poll_cq(cq, 1, &wc) == 0) ;
    if (wc.status != IBV_WC_SUCCESS) die("WRITE failed");
    std::cout << "RDMA WRITE completed\n";

    rdma_disconnect(id);
    ibv_dereg_mr(buf.mr);
    free(buf.addr);
    rdma_destroy_qp(id);
    rdma_destroy_id(id);
    ibv_dealloc_pd(pd);
    ibv_destroy_cq(cq);
    rdma_destroy_event_channel(ec);
    return 0;
}
