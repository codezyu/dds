/*
 * Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License
 */

#pragma once

#include "RDMAController.h"
#include "MsgTypes.h"

class DMABuffer {
private:
    size_t Capacity;
    int ClientId;
    int BufferId;

    //
    // RNIC configuration
    //
    //
    struct ibv_context* ctx;
    struct ibv_pd* pd;
    struct ibv_comp_channel* comp_chan;
    struct ibv_cq* send_cq;
    struct ibv_cq* recv_cq;
    struct ibv_qp* qp;
    struct ibv_mr* mr;

    //
    // Variables for messages
    //
    //
    struct ibv_sge* MsgSgl;
    char MsgBuf[BUFF_MSG_SIZE];
    struct ibv_mr* MsgMr;

public:
    char* BufferAddress;

public:
    DMABuffer(
        const char* _BackEndAddr,
        const unsigned short _BackEndPort,
        const size_t _Capacity,
        const int _ClientId
    );

    //
    // Allocate the buffer with the specified capacity
    // and register it to the NIC;
    // Not thread-safe
    //
    //
    bool
    Allocate(
        struct sockaddr_in* LocalSock,
        struct sockaddr_in* BackEndSock,
        const size_t QueueDepth,
        const size_t MaxSge,
        const size_t InlineThreshold
    );

    //
    // Wait for a completion event
    // Not thread-safe
    //
    //
    void
    WaitForACompletion(
        bool Blocking
    );

    //
    // Release the allocated buffer;
    // Not thread-safe
    //
    //
    void
    Release();
};