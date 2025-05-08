/*
 * RDMC (RDMA Messaging and Control) header for libibverbs-based implementation
 * Converted from Microsoft NetworkDirect SPI version to Linux libibverbs
 * Licensed under MIT
 */

 #pragma once
 #include "Config.h"
 #include <infiniband/verbs.h>
 #include <cstdint>
 #include <cstddef>
 #include <functional>
 
 // Initialize RDMA adapter (device) and create completion channel
 // device_name: name of RDMA device (e.g., "mlx5_0"), port_num: port index (1-based)
 void RDMC_OpenAdapter(
     const char* device_name);
 
 // Create completion queue
 // cqe: maximum number of completions
 // Returns pointer to ibv_cq
 struct ibv_cq* RDMC_CreateCQ(
     int cqe);
 
 // Create a reliable-connected queue pair
 // pd: protection domain
 // send_cq, recv_cq: completion queues
 // max_send_wr, max_recv_wr: max outstanding work requests
 // max_send_sge, max_recv_sge: max scatter/gather entries
 // Returns pointer to ibv_qp
 struct ibv_qp* RDMC_CreateQueuePair(
     struct ibv_pd* pd,
     struct ibv_cq* send_cq,
     struct ibv_cq* recv_cq,
     int max_send_wr,
     int max_recv_wr,
     int max_send_sge,
     int max_recv_sge);
 
 // Register memory region
 // pd: protection domain, buf: buffer pointer, length: size in bytes
 // access_flags: IBV_ACCESS_* flags
 // Returns pointer to ibv_mr
 struct ibv_mr* RDMC_RegisterMemory(
     struct ibv_pd* pd,
     void* buf,
     size_t length,
     int access_flags);
 
 // Wait for and process a completion
 // cq: completion queue
 // blocking: whether to wait for a notification event
 // process_wc: callback to handle the polled work completion entry
 void RDMC_WaitForCompletion(
     struct ibv_cq* cq,
     bool blocking,
     const std::function<void(const struct ibv_wc&)>& process_wc);
 
 // Post a send work request
 // qp: queue pair, sge: scatter-gather list, num_sge: list length, context: user-defined
 void RDMC_Send(
     struct ibv_qp* qp,
     struct ibv_sge* sge,
     int num_sge,
     void* context);
 
 // Post a receive work request
 // qp: queue pair, sge: scatter-gather list, num_sge: list length, context: user-defined
 void RDMC_PostReceive(
     struct ibv_qp* qp,
     struct ibv_sge* sge,
     int num_sge,
     void* context);
 
 // Post an RDMA read
 // qp: queue pair, sge: scatter-gather list, num_sge: list length,
 // remote_addr: remote buffer address, rkey: remote key
 // context: user-defined
 void RDMC_Read(
     struct ibv_qp* qp,
     struct ibv_sge* sge,
     int num_sge,
     uint64_t remote_addr,
     uint32_t rkey,
     void* context);
 
 // Post an RDMA write
 // qp: queue pair, sge: scatter-gather list, num_sge: list length,
 // remote_addr: remote buffer address, rkey: remote key
 // context: user-defined
 void RDMC_Write(
     struct ibv_qp* qp,
     struct ibv_sge* sge,
     int num_sge,
     uint64_t remote_addr,
     uint32_t rkey,
     void* context);
 
 // Close and clean up adapter and resources
 void RDMC_Close();
 