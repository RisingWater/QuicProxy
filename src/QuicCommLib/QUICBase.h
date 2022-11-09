#pragma once

#ifndef __QUIC_BASE_H__
#define __QUIC_BASE_H__

#include "msquic.h"
#include "BaseObject.h"
#include <list>

typedef struct
{
    QUIC_BUFFER Packet;
    HANDLE      SyncHandle;
} QUICSendNode;

typedef struct
{
    DWORD       Length;
    BYTE        Data[0];
} QUICPkt;

typedef enum
{
    QUICBASE_LOW_LATENCY = 0,
    QUICBASE_HIGH_THOUGHPUT,
    QUICBASE_BACKGROUND,
    QUICBASE_REALTIME,
} QUICBASE_TYPE;

class IQUICCommunication;
typedef BOOL(*_QUICRecvPacketProcess)(PBYTE Data, DWORD Length, IQUICCommunication* quic, CBaseObject* Param);
typedef void(*_QUICDisconnectedProcess)(IQUICCommunication* s, CBaseObject* pParam);

class IQUICBase : public CBaseObject
{
public:
    IQUICBase();
    virtual ~IQUICBase();

    virtual BOOL Init() = 0;
    virtual VOID Done() = 0;

protected:
    virtual BOOL BaseInit(QUICBASE_TYPE Type);
    virtual VOID BaseDone();

    const QUIC_API_TABLE* m_pMsQuic;
    QUIC_REGISTRATION_CONFIG m_stRegConfig;
    HQUIC m_hRegistration;
    HQUIC m_hConfiguration;
};

class IQUICCommunication
{
public:
    IQUICCommunication();
    virtual ~IQUICCommunication();

    virtual BOOL SendPacket(PBYTE Data, DWORD Length, HANDLE SyncHandle = NULL) = 0;
    virtual VOID RegisterRecvProcess(_QUICRecvPacketProcess Process, CBaseObject* Param);
    virtual VOID RegisterEndProcess(_QUICDisconnectedProcess Process, CBaseObject* Param);

protected:    
    void ProcessRecvEvent(QUIC_STREAM_EVENT* RecvEvent);
    void ProcessShutdownEvent(QUIC_STREAM_EVENT* ShutdownEvent);
    void ProcessSendCompleteEvent(QUIC_STREAM_EVENT* SendCompleteEvent);

    QUICSendNode* CreateQUICSendNode(PBYTE Data, DWORD Length, HANDLE SyncHandle);

private:
    void AddQuicBufferToDataBuffer(const QUIC_BUFFER* quicBuffer);
    QUICPkt* GetQuicPktFromDataBuffer();

    CRITICAL_SECTION m_csLock;
    _QUICRecvPacketProcess m_pfnRecvFunc;
    _QUICDisconnectedProcess m_pfnEndFunc;
    CBaseObject* m_pRecvParam;
    CBaseObject* m_pEndParam;

    HANDLE m_hDataBufferStream;
};


#endif