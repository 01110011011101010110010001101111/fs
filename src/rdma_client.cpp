#include "rdma_common.hpp"

int main(int argc, char** argv) {
    if (argc < 3) {
        std::cerr << "Usage: " << argv[0] << " <server-ip> <port>\n";
        return EXIT_FAILURE;
    }
    const char* srv_ip = argv[1];
    const char* port   = argv[2];

    /* ---------- 1. RDMA-CM address/route resolution ---------- */
    rdma_event_channel* ec = rdma_create_event_channel();
    rdma_cm_id* id{};
    if (rdma_create_id(ec, &id, nullptr, RDMA_PS_TCP))            die("create_id");
    sockaddr_in dst{};
    dst.sin_family = AF_INET;
    dst.sin_port   = htons(std::stoi(port));
    if (!inet_pton(AF_INET, srv_ip, &dst.sin_addr))               die("inet_pton");
    // get_addr(srv_ip, );

    if (rdma_resolve_addr(id, nullptr,
                          reinterpret_cast<sockaddr*>(&dst), 2000)) die("resolve_addr");
    rdma_cm_event* ev{};
    rdma_get_cm_event(ec, &ev);                                   // ADDR_RESOLVED
    rdma_ack_cm_event(ev);

    if (rdma_resolve_route(id, 2000))                             die("resolve_route");
    rdma_get_cm_event(ec, &ev);                                   // ROUTE_RESOLVED
    rdma_ack_cm_event(ev);

    /* ---------- 2. Verbs objects ---------- */
    ibv_pd* pd  = ibv_alloc_pd(id->verbs);
    ibv_cq* cq  = ibv_create_cq(id->verbs, 16, nullptr, nullptr, 0);

    ibv_qp_init_attr qp_attr{};
    qp_attr.send_cq = qp_attr.recv_cq = cq;
    qp_attr.cap.max_send_wr  = qp_attr.cap.max_recv_wr  = 16;
    qp_attr.cap.max_send_sge = qp_attr.cap.max_recv_sge = 1;
    qp_attr.qp_type = IBV_QPT_RC;
    if (rdma_create_qp(id, pd, &qp_attr))                         die("create_qp");

    /* ---------- 3. Buffer registration & metadata ---------- */
    auto buf = register_memory(pd, 4096);
    std::string meta = std::to_string(reinterpret_cast<uint64_t>(buf.addr)) +
                       ":" + std::to_string(buf.rkey());

    /* ---------- 4. Pre-post a RECV (optional) ---------
       If you expect the server to SEND something back,
       allocate a tiny scratch MR and post it here.       */
    // (not used in this minimal example; server does a WRITE)

    /* ---------- 5. Connect ---------- */
    rdma_conn_param cp{};
    cp.initiator_depth = cp.responder_resources = 1;
    if (rdma_connect(id, &cp))                                    die("connect");

    rdma_get_cm_event(ec, &ev);                                   // ESTABLISHED
    rdma_ack_cm_event(ev);
    std::cout << "Connected to " << srv_ip << ":" << port << "\n";

    /* ---------- 6. SEND our buffer metadata ---------- */
    ibv_sge sge{};
    sge.addr   = reinterpret_cast<uint64_t>(meta.data());
    sge.length = meta.size() + 1;
    sge.lkey   = 0;                       // using a stack buffer -> lkey 0

    ibv_send_wr swr{}, *bad{};
    swr.wr_id      = 1;
    swr.sg_list    = &sge;
    swr.num_sge    = 1;
    swr.opcode     = IBV_WR_SEND;
    swr.send_flags = IBV_SEND_SIGNALED | IBV_SEND_INLINE;

    if (ibv_post_send(id->qp, &swr, &bad))                        die("post_send");

    /* Poll for the SEND completion so we know meta is delivered */
    ibv_wc wc{};
    while (ibv_poll_cq(cq, 1, &wc) == 0) ;
    if (wc.status != IBV_WC_SUCCESS) die("wc failure");

    /* ---------- 7. Wait for server's RDMA WRITE ---------- */
    std::cout << "Waiting for data…\n";
    while (std::string_view(static_cast<char*>(buf.addr), 5) != "Hello")
        ;                     // spin until the server’s WRITE shows up

    std::cout << "Server wrote: '" << static_cast<char*>(buf.addr) << "'\n";

    /* ---------- 8. Graceful shutdown ---------- */
    rdma_disconnect(id);                    // flushes outstanding WRs

    ibv_dereg_mr(buf.mr);
    free(buf.addr);
    rdma_destroy_qp(id);
    ibv_destroy_cq(cq);
    ibv_dealloc_pd(pd);
    rdma_destroy_id(id);
    rdma_destroy_event_channel(ec);
}
