/*
 * Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License
 */

#pragma once

#include "MsgTypes.h"
#include "Protocol.h"
#include "RDMAController.h"
#include <sys/socket.h>
#include <netinet/in.h>
#define BACKEND_TYPE_IN_MEMORY 1
#define BACKEND_TYPE_DMA 2
#define BACKEND_TYPE BACKEND_TYPE_DMA

#define BACKEND_BRIDGE_VERBOSE

//
// Connector that fowards requests to and receives responses from the back end
//
//
class BackEndBridge {
public:
    //
    // Back end configuration
    //
    //
    char BackEndAddr[16];
    unsigned short BackEndPort;
    struct sockaddr_in BackEndSock;

    //
    // RNIC configuration
    //
    //
    struct ibv_context       *ctx;
    struct ibv_pd            *pd;
    struct ibv_comp_channel  *comp_chan;
    struct ibv_cq            *CtrlCompQ;
    struct ibv_qp            *CtrlQPair;
    struct ibv_mr            *CtrlMr;
    struct ibv_sge           CtrlSgl;

    size_t QueueDepth;
    size_t MaxSge;
    size_t InlineThreshold;
    struct sockaddr_in LocalSock;

    char CtrlMsgBuf[CTRL_MSG_SIZE];

    int ClientId;

public:
    BackEndBridge();

    //
    // Connect to the backend
    //
    //
    bool
    Connect();

    bool
    ConnectTest();

    //
    // Disconnect from the backend
    //
    //
    void
    Disconnect();
};
