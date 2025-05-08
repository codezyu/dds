/*
 * RDMC implementation over libibverbs (Linux)
 * Converted from Microsoft NetworkDirect SPI version
 * Licensed under MIT
 */

 #include <arpa/inet.h>
 #include <cstring>
 #include <iostream>
 #include "RDMAController.h"
 
 static struct ibv_context* ctx = nullptr;
 static struct ibv_comp_channel* comp_chan = nullptr;
 
 void RDMC_OpenDevice(
     const char* device_name) {
     // Open device
     ibv_device** dev_list = ibv_get_device_list(nullptr);
     if (!dev_list) {
         perror("ibv_get_device_list"); exit(1);
     }
     ibv_device* dev = nullptr;
     for (int i = 0; dev_list[i]; ++i) {
         if (strcmp(ibv_get_device_name(dev_list[i]), device_name)==0) {
             dev = dev_list[i]; break;
         }
     }
     if (!dev) dev = dev_list[0];
     ctx = ibv_open_device(dev);
     if (!ctx) { perror("ibv_open_device"); exit(1); }
     ibv_free_device_list(dev_list);
     // Create completion channel
     comp_chan = ibv_create_comp_channel(ctx);
     if (!comp_chan) { perror("ibv_create_comp_channel"); exit(1); }
 }
 
 struct ibv_cq* RDMC_CreateCQ(
     int cqe) {
     struct ibv_cq* cq = ibv_create_cq(ctx, cqe, nullptr, comp_chan, 0);
     if (!cq) { perror("ibv_create_cq"); exit(1); }
     ibv_req_notify_cq(cq, 0);
     return cq;
 }
 
 struct ibv_qp* RDMC_CreateQueuePair(
     struct ibv_pd* pd,
     struct ibv_cq* send_cq,
     struct ibv_cq* recv_cq,
     int max_send_wr,
     int max_recv_wr,
     int max_send_sge,
     int max_recv_sge) {
     struct ibv_qp_init_attr qp_init = {};
     qp_init.send_cq = send_cq;
     qp_init.recv_cq = recv_cq;
     qp_init.qp_type = IBV_QPT_RC;
     qp_init.cap.max_send_wr = max_send_wr;
     qp_init.cap.max_recv_wr = max_recv_wr;
     qp_init.cap.max_send_sge = max_send_sge;
     qp_init.cap.max_recv_sge = max_recv_sge;
     struct ibv_qp* qp = ibv_create_qp(pd, &qp_init);
     if (!qp) { perror("ibv_create_qp"); exit(1); }
     return qp;
 }
 
 struct ibv_mr* RDMC_RegisterMemory(
     struct ibv_pd* pd,
     void* buf,
     size_t length,
     int access_flags) {
     struct ibv_mr* mr = ibv_reg_mr(pd, buf, length, access_flags);
     if (!mr) { perror("ibv_reg_mr"); exit(1); }
     return mr;
 }
 
 void RDMC_WaitForCompletion(
     struct ibv_cq* cq,
     bool blocking,
     const std::function<void(const struct ibv_wc&)>& process_wc) {
     struct ibv_cq* ev_cq;
     void* ev_ctx;
     if (blocking) {
         // wait for event
         ibv_get_cq_event(comp_chan, &ev_cq, &ev_ctx);
         ibv_ack_cq_events(ev_cq, 1);
         ibv_req_notify_cq(ev_cq, 0);
     }
     struct ibv_wc wc;
     int n;
     while ((n = ibv_poll_cq(cq, 1, &wc)) == 0) ;
     if (n < 0) { perror("ibv_poll_cq"); exit(1); }
     if (wc.status != IBV_WC_SUCCESS) {
         std::cerr << "Work completion error: " << wc.status << std::endl;
         exit(1);
     }
     process_wc(wc);
 }
 
 void RDMC_Send(
     struct ibv_qp* qp,
     struct ibv_sge* sge,
     int num_sge,
     void* context) {
     struct ibv_send_wr wr = {};
     wr.wr_id = (uintptr_t)context;
     wr.sg_list = sge;
     wr.num_sge = num_sge;
     wr.opcode = IBV_WR_SEND;
     wr.send_flags = IBV_SEND_SIGNALED;
     struct ibv_send_wr* bad_wr;
     int ret = ibv_post_send(qp, &wr, &bad_wr);
     if (ret) { perror("ibv_post_send"); exit(1); }
 }
 
 void RDMC_PostReceive(
     struct ibv_qp* qp,
     struct ibv_sge* sge,
     int num_sge,
     void* context) {
     struct ibv_recv_wr wr = {};
     wr.wr_id = (uintptr_t)context;
     wr.sg_list = sge;
     wr.num_sge = num_sge;
     struct ibv_recv_wr* bad_wr;
     int ret = ibv_post_recv(qp, &wr, &bad_wr);
     if (ret) { perror("ibv_post_recv"); exit(1); }
 }
 
 void RDMC_Read(
     struct ibv_qp* qp,
     struct ibv_sge* sge,
     int num_sge,
     uint64_t remote_addr,
     uint32_t rkey,
     void* context) {
     struct ibv_send_wr wr = {};
     wr.wr_id = (uintptr_t)context;
     wr.sg_list = sge;
     wr.num_sge = num_sge;
     wr.opcode = IBV_WR_RDMA_READ;
     wr.send_flags = IBV_SEND_SIGNALED;
     wr.wr.rdma.remote_addr = remote_addr;
     wr.wr.rdma.rkey = rkey;
     struct ibv_send_wr* bad_wr;
     int ret = ibv_post_send(qp, &wr, &bad_wr);
     if (ret) { perror("ibv_post_send RDMA_READ"); exit(1); }
 }
 
 void RDMC_Write(
     struct ibv_qp* qp,
     struct ibv_sge* sge,
     int num_sge,
     uint64_t remote_addr,
     uint32_t rkey,
     void* context) {
     struct ibv_send_wr wr = {};
     wr.wr_id = (uintptr_t)context;
     wr.sg_list = sge;
     wr.num_sge = num_sge;
     wr.opcode = IBV_WR_RDMA_WRITE;
     wr.send_flags = IBV_SEND_SIGNALED;
     wr.wr.rdma.remote_addr = remote_addr;
     wr.wr.rdma.rkey = rkey;
     struct ibv_send_wr* bad_wr;
     int ret = ibv_post_send(qp, &wr, &bad_wr);
     if (ret) { perror("ibv_post_send RDMA_WRITE"); exit(1); }
 }
 
 void RDMC_Close() {
     if (ctx) {
         ibv_destroy_comp_channel(comp_chan);
         ibv_close_device(ctx);
         ctx = nullptr;
     }
 }
 