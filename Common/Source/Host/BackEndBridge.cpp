#include "BackEndBridge.h"
#include <netdb.h>
#include <unistd.h>
#include <iostream>
extern struct ibv_context* ctx;
extern struct ibv_comp_channel* comp_chan;
#define INLINE_THRESHOLD 1024
BackEndBridge::BackEndBridge() {
    // Record backend address and port
    strcpy(BackEndAddr, DDS_BACKEND_ADDR);
    BackEndPort = DDS_BACKEND_PORT;
    memset(&BackEndSock, 0, sizeof(BackEndSock));

    // Initialize RDMA resources
    pd         = nullptr;
    CtrlCompQ  = nullptr;
    CtrlQPair  = nullptr;
    CtrlMr     = nullptr;
    memset(CtrlMsgBuf, 0, CTRL_MSG_SIZE);

    ClientId = -1;
}

bool BackEndBridge::Connect() {
    // Resolve backend hostname
    struct hostent* he = gethostbyname(BackEndAddr);
    if (!he) {
        perror("gethostbyname");
        return false;
    }
    BackEndSock.sin_family = AF_INET;
    memcpy(&BackEndSock.sin_addr, he->h_addr_list[0], he->h_length);
    BackEndSock.sin_port = htons(BackEndPort);

    // Open RDMA device and completion channel
    RDMC_OpenAdapter(MLX_DEVICE);

    // Query device capabilities
    struct ibv_device_attr dev_attr;
    if (ibv_query_device(ctx, &dev_attr)) {
        perror("ibv_query_device");
        return false;
    }
    QueueDepth     = dev_attr.max_cqe;
    MaxSge         = dev_attr.max_sge;
    InlineThreshold= INLINE_THRESHOLD;

    // Allocate Protection Domain
    pd = ibv_alloc_pd(ctx);
    if (!pd) {
        perror("ibv_alloc_pd");
        return false;
    }

    // Create Completion Queue
    CtrlCompQ = RDMC_CreateCQ(QueueDepth);

    // Create Queue Pair (Reliable Connected)
    CtrlQPair = RDMC_CreateQueuePair(
        pd, CtrlCompQ, CtrlCompQ,
        QueueDepth, QueueDepth,
        MaxSge, MaxSge
    );

    // Register control message memory region
    CtrlMr = RDMC_RegisterMemory(
        pd,
        CtrlMsgBuf,
        CTRL_MSG_SIZE,
        IBV_ACCESS_LOCAL_WRITE |
        IBV_ACCESS_REMOTE_READ |
        IBV_ACCESS_REMOTE_WRITE
    );

    // Prepare SGE for control messages
    CtrlSgl.addr   = reinterpret_cast<uintptr_t>(CtrlMsgBuf);
    CtrlSgl.length = CTRL_MSG_SIZE;
    CtrlSgl.lkey   = CtrlMr->lkey;

    // Handshake: post receive, send request, wait for response
    RDMC_PostReceive(CtrlQPair, &CtrlSgl, 1, reinterpret_cast<void*>(MSG_CTXT));

    ((MsgHeader*)CtrlMsgBuf)->MsgId = CTRL_MSG_F2B_REQUEST_ID;
    RDMC_Send(CtrlQPair, &CtrlSgl, 1, reinterpret_cast<void*>(MSG_CTXT));

    // Wait for send completion
    RDMC_WaitForCompletion(CtrlCompQ, true, [](const struct ibv_wc&){ });
    // Wait for receive completion
    RDMC_WaitForCompletion(CtrlCompQ, true, [](const struct ibv_wc&){ });

    // Parse response
    if (((MsgHeader*)CtrlMsgBuf)->MsgId != CTRL_MSG_B2F_RESPOND_ID) {
        std::cerr << "BackEndBridge: unexpected response" << std::endl;
        return false;
    }
    ClientId = ((CtrlMsgB2FRespondId*)(CtrlMsgBuf + sizeof(MsgHeader)))->ClientId;
    std::cout << "BackEndBridge: connected, client ID=" << ClientId << std::endl;

    return true;
}

bool BackEndBridge::ConnectTest() {
    // Alias to Connect for libibverbs
    return Connect();
}

void BackEndBridge::Disconnect() {
    if (ClientId >= 0) {
        ((MsgHeader*)CtrlMsgBuf)->MsgId = CTRL_MSG_F2B_TERMINATE;
        ((CtrlMsgF2BTerminate*)(CtrlMsgBuf + sizeof(MsgHeader)))->ClientId = ClientId;
        CtrlSgl.length = sizeof(MsgHeader) + sizeof(CtrlMsgF2BTerminate);
        RDMC_Send(CtrlQPair, &CtrlSgl, 1, reinterpret_cast<void*>(MSG_CTXT));
        RDMC_WaitForCompletion(CtrlCompQ, true, [](const struct ibv_wc&){ });
        std::cout << "BackEndBridge: sent terminate" << std::endl;
    }

    // Cleanup RDMA resources
    if (CtrlMr) ibv_dereg_mr(CtrlMr);
    if (CtrlQPair) ibv_destroy_qp(CtrlQPair);
    if (CtrlCompQ) ibv_destroy_cq(CtrlCompQ);
    if (pd) ibv_dealloc_pd(pd);

    // Close adapter
    RDMC_Close();
}