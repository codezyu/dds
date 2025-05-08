/*
 * Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License
 */

#pragma once

#include "DDSBackEndBridgeBase.h"
#include "RDMAController.h"
#include <sys/socket.h>
#include <netinet/in.h>
#undef CreateDirectory
#undef RemoveDirectory
#undef CreateFile
#undef DeleteFile
#undef FindFirstFile
#undef FindNextFile
#undef GetFileAttributes
#undef GetCurrentDirectory
#undef MoveFile

namespace DDS_FrontEnd {

//
// Connector that fowards requests to and receives responses from the back end
//
//
class DDSBackEndBridge : public DDSBackEndBridgeBase {
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

#ifdef RING_BUFFER_RESPONSE_BATCH_ENABLED
    //
    // Caching a received batch
    //
    //
    SplittableBufferT BatchRef;
    FileIOSizeT ProcessedBytes;
    BufferT NextResponse;
#endif

public:
    DDSBackEndBridge();

    //
    // Connect to the backend
    //
    //
    ErrorCodeT
    Connect();

    //
    // Disconnect from the backend
    //
    //
    ErrorCodeT
    Disconnect();

	//
    // Create a diretory
    // 
    //
    ErrorCodeT
    CreateDirectory(
        const char* PathName,
        DirIdT DirId,
        DirIdT ParentId
    );

    //
    // Delete a directory
    //
    //
    ErrorCodeT
    RemoveDirectory(
        DirIdT DirId
    );

    //
    // Create a file
    // 
    //
    ErrorCodeT
    CreateFile(
        const char* FileName,
        FileAttributesT FileAttributes,
        FileIdT FileId,
        DirIdT DirId
    );

    //
    // Delete a file
    // 
    //
    ErrorCodeT
    DeleteFile(
        FileIdT FileId,
        DirIdT DirId
    );

    //
    // Change the size of a file
    // 
    //
    ErrorCodeT
    ChangeFileSize(
        FileIdT FileId,
        FileSizeT NewSize
    );

    //
    // Get file size
    // 
    //
    ErrorCodeT
    GetFileSize(
        FileIdT FileId,
        FileSizeT* FileSize
    );

    //
    // Async read from a file
    // 
    //
    ErrorCodeT
    ReadFile(
        FileIdT FileId,
        FileSizeT Offset,
        BufferT DestBuffer,
        FileIOSizeT BytesToRead,
        FileIOSizeT* BytesRead,
        ContextT Context,
        PollT* Poll
    );

    //
    // Async read from a file with scattering
    // 
    //
    ErrorCodeT
    ReadFileScatter(
        FileIdT FileId,
        FileSizeT Offset,
        BufferT* DestBufferArray,
        FileIOSizeT BytesToRead,
        FileIOSizeT* BytesRead,
        ContextT Context,
        PollT* Poll
    );

    //
    // Async write to a file
    // 
    //
    ErrorCodeT
    WriteFile(
        FileIdT FileId,
        FileSizeT Offset,
        BufferT SourceBuffer,
        FileIOSizeT BytesToWrite,
        FileIOSizeT* BytesWritten,
        ContextT Context,
        PollT* Poll
    );

    //
    // Async write to a file with gathering
    // 
    //
    ErrorCodeT
    WriteFileGather(
        FileIdT FileId,
        FileSizeT Offset,
        BufferT* SourceBufferArray,
        FileIOSizeT BytesToWrite,
        FileIOSizeT* BytesWritten,
        ContextT Context,
        PollT* Poll
    );

    //
    // Get file properties by file id
    // 
    //
    ErrorCodeT
    GetFileInformationById(
        FileIdT FileId,
        FilePropertiesT* FileProperties
    );

    //
    // Get file attributes by file name
    // 
    //
    ErrorCodeT
    GetFileAttributes(
        FileIdT FileId,
        FileAttributesT* FileAttributes
    );

    //
    // Get the size of free space on the storage
    // 
    //
    ErrorCodeT
    GetStorageFreeSpace(
        FileSizeT* StorageFreeSpace
    );

    //
    // Move an existing file or a directory,
    // including its children
    // 
    //
    ErrorCodeT
    MoveFile(
        FileIdT FileId,
        const char* NewFileName
    );

    //
    // Retrieve a response from the response ring
    // 
    //
    ErrorCodeT
    GetResponse(
        PollT* Poll,
        size_t WaitTime,
        FileIOSizeT* BytesServiced,
        RequestIdT* ReqId
    );

#ifdef RING_BUFFER_RESPONSE_BATCH_ENABLED
    //
    // Retrieve a response from the cached batch
    //
    //
    ErrorCodeT
    GetResponseFromCachedBatch(
        PollT* Poll,
        FileIOSizeT* BytesServiced,
        RequestIdT* ReqId
    );
#endif
};

}