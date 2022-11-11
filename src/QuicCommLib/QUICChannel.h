#ifndef __QUIC_CHANNEL_H__
#define __QUIC_CHANNEL_H__

#include "QUICBase.h"
#include "QUICCommLib.h"

#define CTRL_CHANNEL_NAME "CTRL_CHANNEL"

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

class CQUICChannel;

class CQUICChannel : public IQUICChannel
{
public:
    CQUICChannel(HQUIC hStream, const QUIC_API_TABLE* pMsQuicAPI);

    CQUICChannel(const QUIC_API_TABLE* pMsQuicAPI, CHAR* ChannelName, WORD Priority);

    LONG GetChannelID();

    BOOL Init(HQUIC hConnection);

    void Done();

    virtual const CHAR* GetChannelName();

    virtual WORD GetPriority();

    virtual BOOL SendPacket(PBYTE Data, DWORD Length, HANDLE SyncHandle = NULL);

    virtual VOID RegisterRecvProcess(_QUICRecvPacketProcess Process, CBaseObject* Param);

    virtual VOID RegisterEndProcess(_QUICDisconnectedProcess Process, CBaseObject* Param);

private:
    BOOL SendPacket(PBYTE Data, DWORD Length, HANDLE SyncHandle, QUIC_SEND_FLAGS SendFlags);

    virtual ~CQUICChannel();

    void ProcessRecvEvent(QUIC_STREAM_EVENT* RecvEvent);

    void ProcessShutdownEvent(QUIC_STREAM_EVENT* ShutdownEvent);

    void ProcessSendCompleteEvent(QUIC_STREAM_EVENT* SendCompleteEvent);

    QUICSendNode* CreateQUICSendNode(PBYTE Data, DWORD Length, HANDLE SyncHandle);

    void AddQuicBufferToDataBuffer(const QUIC_BUFFER* quicBuffer);

    QUICPkt* GetQuicPktFromDataBuffer();

    static QUIC_STATUS QUIC_API ClientStreamCallback(
        _In_ HQUIC Stream,
        _In_opt_ void* Context,
        _Inout_ QUIC_STREAM_EVENT* Event
        );

    HQUIC                    m_hStream;

    const QUIC_API_TABLE*    m_pMsQuic;

    CHAR                     m_szChannelName[256];

    WORD                     m_wPriority;

    CRITICAL_SECTION         m_csLock;

    _QUICRecvPacketProcess   m_pfnRecvFunc;

    _QUICDisconnectedProcess m_pfnEndFunc;

    CBaseObject*             m_pRecvParam;

    CBaseObject*             m_pEndParam;

    HANDLE                   m_hDataBufferStream;

    BOOL                     m_bHasChannelInfo;

    LONG                     m_iChannelId;

    BOOL                     m_bIsClient;
};

#endif