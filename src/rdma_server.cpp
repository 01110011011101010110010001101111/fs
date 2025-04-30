#include "rdma_common.hpp"

int main(int argc, char** argv) {
    const char* port = (argc > 1) ? argv[1] : "7471";

    rdma_cm_id* listener{};
    rdma_event_channel* ec = rdma_create_event_channel();
    if (rdma_create_id(ec, &listener, nullptr, RDMA_PS_TCP)) die("create_id");
    sockaddr_in addr { .sin_family = AF_INET, .sin_port = htons(atoi(port)) };
    if (rdma_bind_addr(listener, reinterpret_cast<sockaddr*>(&addr))) die("bind");
    if (rdma_listen(listener, 1)) die("listen");
    std::cout << "Listening on " << port << "…\n";

    rdma_cm_event* event{};
    rdma_get_cm_event(ec, &event);              // RDMA_CM_EVENT_CONNECT_REQUEST
    rdma_cm_id* id = event->id;
    rdma_ack_cm_event(event);

    // 1. Create verbs objects on this connection
    ibv_pd*  pd  = ibv_alloc_pd(id->verbs);
    ibv_cq*  cq  = ibv_create_cq(id->verbs, 16, nullptr, nullptr, 0);
    ibv_qp_init_attr qp_attr {};
    qp_attr.send_cq = qp_attr.recv_cq = cq;
    qp_attr.cap.max_send_wr  = qp_attr.cap.max_recv_wr  = 16;
    qp_attr.cap.max_send_sge = qp_attr.cap.max_recv_sge = 1;
    qp_attr.qp_type = IBV_QPT_RC;
    rdma_create_qp(id, pd, &qp_attr);

    // 2. Register a 4 KiB buffer and share its addr+rkey with the peer
    auto buf = register_memory(pd, 4096);
    std::string metadata = std::to_string(reinterpret_cast<uint64_t>(buf.addr)) +
                           ":" + std::to_string(buf.rkey());
    rdma_accept(id, nullptr);                   // bring QP to RTS

    // 3. Wait until peer sends its metadata in a SEND
    ibv_recv_wr  rwr { .wr_id = 1 };
    ibv_sge      sge { .addr = reinterpret_cast<uint64_t>(buf.addr),
                       .length = 64, .lkey = buf.mr->lkey };
    rwr.sg_list = &sge; rwr.num_sge = 1;
    ibv_post_recv(id->qp, &rwr, nullptr);

    ibv_wc wc{};
    while (ibv_poll_cq(cq, 1, &wc) == 0) ;
    assert(wc.opcode == IBV_WC_RECV);
    std::cout << "Received peer info: " << static_cast<char*>(buf.addr) << "\n";

    // 4. Parse peer’s addr/rkey, then issue one-sided WRITE
    uint64_t peer_addr; uint32_t peer_rkey;
    sscanf(static_cast<char*>(buf.addr), "%lu:%u", &peer_addr, &peer_rkey);
    strcpy(static_cast<char*>(buf.addr), "Hello via RDMA WRITE!");

    ibv_send_wr swr{}, *bad{};
    ibv_sge wsge { .addr = reinterpret_cast<uint64_t>(buf.addr),
                   .length = 24, .lkey = buf.mr->lkey };
    swr.wr_id   = 2;
    swr.sg_list = &wsge; swr.num_sge = 1;
    swr.opcode  = IBV_WR_RDMA_WRITE;
    swr.send_flags = IBV_SEND_SIGNALED;
    swr.wr.rdma.remote_addr = peer_addr;
    swr.wr.rdma.rkey        = peer_rkey;

    ibv_post_send(id->qp, &swr, &bad);
    while (ibv_poll_cq(cq, 1, &wc) == 0) ;
    assert(wc.status == IBV_WC_SUCCESS);
    std::cout << "WRITE completed\n";

    rdma_disconnect(id);
    // …destroy QP, CQ, PD, dereg MR, free buf, etc.
}
