/*
 * Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License
 */

#include "DMABuffer.h"
#include "Config.h"
#include <cstdio>
#include <cstdlib>
#include <arpa/inet.h>
#include <iostream>

DMABuffer::DMABuffer(
    const char* _BackEndAddr,
    const unsigned short _BackEndPort,
    const size_t _Capacity,
    const int _ClientId
) {
    Capacity    = _Capacity;
    ClientId    = _ClientId;
    BufferId    = -1;
    BufferAddress = nullptr;
    MsgSgl      = nullptr;
    MsgMr       = nullptr;
    pd          = nullptr;
    send_cq     = recv_cq = nullptr;
    qp          = nullptr;
    mr          = nullptr;
    memset(MsgBuf, 0, BUFF_MSG_SIZE);
}

bool DMABuffer::Allocate(
    struct sockaddr_in* LocalSock,
    struct sockaddr_in* BackEndSock,
    const size_t QueueDepth,
    const size_t MaxSge,
    const size_t InlineThreshold
) {
    // Open RDMA adapter (default device)
    RDMC_OpenAdapter(MLX_DEVICE);
    // Create PD
    pd = ibv_alloc_pd(ctx);
    if (!pd) { perror("ibv_alloc_pd"); return false; }
    // Create CQs
    send_cq = RDMC_CreateCQ((int)QueueDepth);
    recv_cq = send_cq;
    // Create QP
    qp = RDMC_CreateQueuePair(pd, send_cq, recv_cq,
                              (int)QueueDepth, (int)QueueDepth,
                              (int)MaxSge, (int)MaxSge);
    // Allocate and register DMA buffer
    BufferAddress = (char*)malloc(Capacity);
    if (!BufferAddress) { perror("malloc buffer"); return false; }
    memset(BufferAddress, 0, Capacity);
    mr = RDMC_RegisterMemory(pd, BufferAddress, Capacity,
                              IBV_ACCESS_LOCAL_WRITE |
                              IBV_ACCESS_REMOTE_READ |
                              IBV_ACCESS_REMOTE_WRITE);
    // Register message buffer
    MsgMr = RDMC_RegisterMemory(pd, MsgBuf, BUFF_MSG_SIZE,
                                IBV_ACCESS_LOCAL_WRITE |
                                IBV_ACCESS_REMOTE_READ |
                                IBV_ACCESS_REMOTE_WRITE);
    // Prepare SGE
    MsgSgl = new ibv_sge[1];
    MsgSgl[0].addr   = (uintptr_t)MsgBuf;
    MsgSgl[0].length = BUFF_MSG_SIZE;
    MsgSgl[0].lkey   = MsgMr->lkey;
    // Handshake: post receive
    RDMC_PostReceive(qp, MsgSgl, 1, (void*)(uintptr_t)MSG_CTXT);
    // Build request
    auto hdr = reinterpret_cast<MsgHeader*>(MsgBuf);
    hdr->MsgId = BUFF_MSG_F2B_REQUEST_ID;
    auto req = reinterpret_cast<BuffMsgF2BRequestId*>(MsgBuf + sizeof(MsgHeader));
    req->ClientId     = ClientId;
    req->BufferAddress= (uint64_t)BufferAddress;
    req->Capacity     = (uint32_t)Capacity;
    req->AccessToken  = htonl(mr->rkey);
    MsgSgl[0].length  = sizeof(MsgHeader) + sizeof(BuffMsgF2BRequestId);
    // Send request
    RDMC_Send(qp, MsgSgl, 1, (void*)(uintptr_t)MSG_CTXT);
    RDMC_WaitForCompletion(send_cq, true, [](auto const&){ });
    RDMC_WaitForCompletion(recv_cq, true, [](auto const&){ });
    // Parse response
    hdr = reinterpret_cast<MsgHeader*>(MsgBuf);
    if (hdr->MsgId != BUFF_MSG_B2F_RESPOND_ID) {
        std::cerr << "DMABuffer: bad response" << std::endl;
        return false;
    }
    auto resp = reinterpret_cast<BuffMsgB2FRespondId*>(MsgBuf + sizeof(MsgHeader));
    BufferId = resp->BufferId;
    std::cout << "DMABuffer: assigned buffer id=" << BufferId << std::endl;
    // Restore full SGE length
    MsgSgl[0].length = BUFF_MSG_SIZE;
    // Pre-post receives for notifications
    for (int i = 0; i < DDS_MAX_COMPLETION_BUFFERING; ++i) {
        RDMC_PostReceive(qp, MsgSgl, 1, (void*)(uintptr_t)MSG_CTXT);
    }
    return true;
}

void DMABuffer::WaitForACompletion(bool Blocking) {
    RDMC_WaitForCompletion(recv_cq, Blocking, [](auto const&){ });
    // Re-post receive
    RDMC_PostReceive(qp, MsgSgl, 1, (void*)(uintptr_t)MSG_CTXT);
}

void DMABuffer::Release() {
    if (BufferId >= 0) {
        auto hdr = reinterpret_cast<MsgHeader*>(MsgBuf);
        hdr->MsgId = BUFF_MSG_F2B_RELEASE;
        auto rel = reinterpret_cast<BuffMsgF2BRelease*>(MsgBuf + sizeof(MsgHeader));
        rel->ClientId = ClientId;
        rel->BufferId = BufferId;
        MsgSgl[0].length = sizeof(MsgHeader) + sizeof(BuffMsgF2BRelease);
        RDMC_Send(qp, MsgSgl, 1, (void*)(uintptr_t)MSG_CTXT);
        RDMC_WaitForCompletion(send_cq, true, [](auto const&){ });
    }
    // Cleanup resources
    delete[] MsgSgl;
    if (MsgMr)    ibv_dereg_mr(MsgMr);
    if (mr)       ibv_dereg_mr(mr);
    if (qp)       ibv_destroy_qp(qp);
    if (send_cq)  ibv_destroy_cq(send_cq);
    if (pd)       ibv_dealloc_pd(pd);
    if (BufferAddress) free(BufferAddress);
    // Close adapter
    RDMC_Close();
}
