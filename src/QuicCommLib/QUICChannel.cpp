#include "stdafx.h"
#include "QUICChannel.h"
#include "corelib.h"

#define STREAM_PRIORITY_DEFAULT 0x7FFF

typedef struct
{
    BYTE ChannelName[256];
    WORD Priority;
} CHANNEL_INIT_PKT;

static volatile LONG g_ChannelId = 0;

CQUICChannel::CQUICChannel(HQUIC hStream, const QUIC_API_TABLE* pMsQuicAPI)
    : CBaseObject()
{
    m_hStream = hStream;
    m_pMsQuic = pMsQuicAPI;

    memset(m_szChannelName, 0, 256);
    m_wPriority = 0;

    m_pfnRecvFunc = NULL;
    m_pRecvParam = NULL;

    m_pfnEndFunc = NULL;
    m_pEndParam = NULL;

    m_hDataBufferStream = CreateDataStreamBuffer();

    InitializeCriticalSection(&m_csLock);

    m_iChannelId = -1;
    m_bHasChannelInfo = FALSE;

    m_bIsClient = FALSE;
}

CQUICChannel::CQUICChannel(const QUIC_API_TABLE* pMsQuicAPI, const CHAR* ChannelName, WORD Priority)
    : CBaseObject()
{
    m_hStream = NULL;
    m_pMsQuic = pMsQuicAPI;

    strcpy(m_szChannelName, ChannelName);
    m_wPriority = Priority + STREAM_PRIORITY_DEFAULT;

    m_pfnRecvFunc = NULL;
    m_pRecvParam = NULL;

    m_pfnEndFunc = NULL;
    m_pEndParam = NULL;

    m_hDataBufferStream = CreateDataStreamBuffer();

    InitializeCriticalSection(&m_csLock);

    m_iChannelId = InterlockedIncrement(&g_ChannelId);
    m_bHasChannelInfo = TRUE;

    m_bIsClient = TRUE;
}

CQUICChannel::~CQUICChannel()
{
    CloseDataStreamBuffer(m_hDataBufferStream);

    DeleteCriticalSection(&m_csLock);
}

const CHAR* CQUICChannel::GetChannelName()
{
    return m_szChannelName;
}

WORD CQUICChannel::GetPriority()
{
    return m_wPriority;
}

LONG CQUICChannel::GetChannelID()
{
    return m_iChannelId;
}

BOOL CQUICChannel::Init(HQUIC hConnection)
{
    if (m_hStream == NULL)
    {
        QUIC_STATUS Status;
        CHANNEL_INIT_PKT pkt;
        HANDLE DoneEvent = NULL;

        AddRef();
        Status = m_pMsQuic->StreamOpen(hConnection, QUIC_STREAM_OPEN_FLAG_NONE, CQUICChannel::ClientStreamCallback, this, &m_hStream);
        if (QUIC_FAILED(Status))
        {
            DBG_ERROR(_T("StreamOpen failed, 0x%x!\n"), Status);
            m_hStream = NULL;
            Release();
            goto Error;
        }

        DBG_INFO(_T("[strm][%p] Starting...\n"), m_hStream);

        Status = m_pMsQuic->StreamStart(m_hStream, QUIC_STREAM_START_FLAG_IMMEDIATE);

        if (QUIC_FAILED(Status))
        {
            DBG_ERROR(_T("StreamStart failed, 0x%x!\n"), Status);
            goto Error;
        }

        Status = m_pMsQuic->SetParam(m_hStream, QUIC_PARAM_STREAM_PRIORITY, sizeof(WORD), &m_wPriority);

        if (QUIC_FAILED(Status))
        {
            DBG_ERROR(_T("SetParam QUIC_PARAM_STREAM_PRIORITY failed, 0x%x!\n"), Status);
        }

        memset(&pkt, 0, sizeof(CHANNEL_INIT_PKT));
        strcpy((char*)pkt.ChannelName, m_szChannelName);
        pkt.Priority = m_wPriority;

#ifdef UNICODE
        DBG_TRACE(_T("QUICChannel Send Channel Name %S\n"), m_szChannelName);
#else
        DBG_TRACE(_T("QUICChannel Send Channel Name %s\n"), m_szChannelName);
#endif

        DoneEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
        if (SendPacket((PBYTE)&pkt, sizeof(CHANNEL_INIT_PKT), DoneEvent))
        {
            DWORD Ret = WaitForSingleObject(DoneEvent, 1000);
            if (Ret != WAIT_OBJECT_0)
            {
                goto Error;
            }
        }
        else
        {
            goto Error;
        }
        
        return TRUE;

    Error:
        if (DoneEvent)
        {
            CloseHandle(DoneEvent);
        }

        if (m_hStream)
        {
            m_pMsQuic->StreamClose(m_hStream);
        }

        return FALSE;
    }
    else
    {
        AddRef();
        m_pMsQuic->SetCallbackHandler(m_hStream, (void*)CQUICChannel::ClientStreamCallback, this);

        return TRUE;
    }
}

void CQUICChannel::Done()
{
    DBG_INFO(_T("Sending CloseConnection...\n"));

    DWORD Close = 0;

    RegisterRecvProcess(NULL, NULL);
    RegisterEndProcess(NULL, NULL);

    if (!SendPacket((PBYTE)&Close, sizeof(DWORD), NULL, QUIC_SEND_FLAG_FIN))
    {
        m_pMsQuic->StreamShutdown(m_hStream, QUIC_STREAM_SHUTDOWN_FLAG_ABORT, 0);
        return;
    }

    return;
}

QUIC_STATUS CQUICChannel::ClientStreamCallback(
    _In_ HQUIC Stream,
    _In_opt_ void* Context,
    _Inout_ QUIC_STREAM_EVENT* Event
    )
{
    CQUICChannel* pChannel = (CQUICChannel*)Context;

    switch (Event->Type)
    {
    case QUIC_STREAM_EVENT_SEND_COMPLETE:
    {
        if (pChannel)
        {
            pChannel->ProcessSendCompleteEvent(Event);
        }

        break;
    }
    case QUIC_STREAM_EVENT_RECEIVE:
    {
        if (pChannel)
        {
            pChannel->ProcessRecvEvent(Event);
        }
        break;
    }
    case QUIC_STREAM_EVENT_PEER_SEND_ABORTED:
    {
        DBG_INFO(_T("[strm][%p] Peer aborted\n"), Stream);
        if (pChannel && !pChannel->m_bIsClient)
        {
            pChannel->m_pMsQuic->StreamShutdown(Stream, QUIC_STREAM_SHUTDOWN_FLAG_ABORT, 0);
        }
        break;
    }
    case QUIC_STREAM_EVENT_PEER_SEND_SHUTDOWN:
    {
        DBG_INFO(_T("[strm][%p] Peer shut down\n"), Stream);
        if (pChannel && !pChannel->m_bIsClient)
        {
            pChannel->Done();
        }
        break;
    }
    case QUIC_STREAM_EVENT_SHUTDOWN_COMPLETE:
    {
        DBG_INFO(_T("[strm][%p] All done\n"), Stream);
        if (!Event->SHUTDOWN_COMPLETE.AppCloseInProgress)
        {
            if (pChannel)
            {
                pChannel->ProcessShutdownEvent(Event);
                pChannel->m_pMsQuic->StreamClose(Stream);
                pChannel->Release();
            }
        }
        break;
    }
    default:
    {
        break;
    }
    }

    return QUIC_STATUS_SUCCESS;
}

BOOL CQUICChannel::SendPacketSync(PBYTE Data, DWORD Length)
{
    HANDLE DoneEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
    BOOL Ret = SendPacket(Data, Length, DoneEvent);

    if (Ret)
    {
        WaitForSingleObject(DoneEvent, INFINITE);
    }

    CloseHandle(DoneEvent);

    return Ret;
}

BOOL CQUICChannel::SendPacket(PBYTE Data, DWORD Length, HANDLE SyncHandle)
{
    return SendPacket(Data, Length, SyncHandle, QUIC_SEND_FLAG_NONE);
}

BOOL CQUICChannel::SendPacket(PBYTE Data, DWORD Length, HANDLE SyncHandle, QUIC_SEND_FLAGS SendFlags)
{
    QUICSendNode* Node = CreateQUICSendNode(Data, Length, SyncHandle);
    
    QUIC_STATUS Status = m_pMsQuic->StreamSend(m_hStream, &Node->Packet, 1, SendFlags, Node);

    if (QUIC_FAILED(Status))
    {
        DBG_ERROR(_T("StreamSend failed, 0x%x!\n"), Status);
        free(Node);
        return FALSE;
    }

    return TRUE;
}

void CQUICChannel::ProcessRecvEvent(QUIC_STREAM_EVENT* RecvEvent)
{
    for (unsigned int i = 0; i < RecvEvent->RECEIVE.BufferCount; i++)
    {
        AddQuicBufferToDataBuffer(&RecvEvent->RECEIVE.Buffers[i]);
    }

    while (TRUE)
    {
        QUICPkt* pkt = GetQuicPktFromDataBuffer();
        if (pkt != NULL)
        {
            if (!m_bHasChannelInfo)
            {
                CHANNEL_INIT_PKT* ChannelInit = (CHANNEL_INIT_PKT*)pkt->Data;
                strcpy(m_szChannelName, (char*)ChannelInit->ChannelName);
                m_wPriority = ChannelInit->Priority;

#ifdef UNICODE
                DBG_TRACE(_T("QUICChannel Recv Channel Name %S\n"), m_szChannelName);
#else
                DBG_TRACE(_T("QUICChannel Recv Channel Name %s\n"), m_szChannelName);
#endif

                m_iChannelId = InterlockedIncrement(&g_ChannelId);
                m_bHasChannelInfo = TRUE;
            }
            else
            {
                EnterCriticalSection(&m_csLock);
                if (m_pfnRecvFunc)
                {
                    m_pfnRecvFunc(pkt->Data, pkt->Length - sizeof(QUICPkt), this, m_pRecvParam);
                }
                LeaveCriticalSection(&m_csLock);
            }
            free(pkt);
        }
        else
        {
            break;
        }
    }

    return;
}

void CQUICChannel::ProcessShutdownEvent(QUIC_STREAM_EVENT* ShutdownEvent)
{
    EnterCriticalSection(&m_csLock);

    if (m_pfnEndFunc)
    {
        m_pfnEndFunc(this, m_pEndParam);
    }

    LeaveCriticalSection(&m_csLock);

    return;
}

void CQUICChannel::ProcessSendCompleteEvent(QUIC_STREAM_EVENT* SendCompleteEvent)
{
    QUICSendNode* Node = (QUICSendNode*)(SendCompleteEvent->SEND_COMPLETE.ClientContext);

    if (Node->SyncHandle)
    {
        SetEvent(Node->SyncHandle);
    }

    free(Node);

    return;
}

QUICSendNode* CQUICChannel::CreateQUICSendNode(PBYTE Data, DWORD Length, HANDLE SyncHandle)
{
    QUICSendNode* Node = (QUICSendNode*)malloc(sizeof(QUICSendNode) + sizeof(QUICPkt) + Length);

    Node->Packet.Buffer = (uint8_t*)Node + sizeof(QUICSendNode);
    Node->Packet.Length = sizeof(QUICPkt) + Length;
    Node->SyncHandle = SyncHandle;

    QUICPkt* QuicPacket = (QUICPkt*)Node->Packet.Buffer;
    QuicPacket->Length = sizeof(QUICPkt) + Length;
    memcpy(QuicPacket->Data, Data, Length);

    return Node;
}

void CQUICChannel::AddQuicBufferToDataBuffer(const QUIC_BUFFER* quicBuffer)
{
    DataStreamBufferAddData(m_hDataBufferStream, quicBuffer->Buffer, quicBuffer->Length);
    return;
}

QUICPkt* CQUICChannel::GetQuicPktFromDataBuffer()
{
    QUICPkt* pPacket = NULL;
    DWORD dwReaded = 0;
    DWORD dwCurrentSize = DataStreamBufferGetCurrentDataSize(m_hDataBufferStream);
    if (dwCurrentSize >= sizeof(QUICPkt))
    {
        QUICPkt header;

        DataStreamBufferGetData(m_hDataBufferStream, (BYTE*)&header, sizeof(QUICPkt), &dwReaded);
        DataStreamBufferAddDataFront(m_hDataBufferStream, (BYTE*)&header, dwReaded);

        if (dwReaded == sizeof(QUICPkt))
        {
            if (header.Length <= dwCurrentSize)
            {
                pPacket = (QUICPkt*)malloc(header.Length);
                DataStreamBufferGetData(m_hDataBufferStream, (BYTE*)pPacket, header.Length, &dwReaded);

                if (dwReaded != header.Length)
                {
                    DataStreamBufferAddDataFront(m_hDataBufferStream, (BYTE*)pPacket, dwReaded);
                    free(pPacket);
                    pPacket = NULL;
                }
            }
        }
    }

    return pPacket;
}

VOID CQUICChannel::RegisterRecvProcess(_QUICRecvPacketProcess Process, CBaseObject* Param)
{
    CBaseObject* pOldParam = NULL;

    EnterCriticalSection(&m_csLock);
    m_pfnRecvFunc = Process;

    pOldParam = m_pRecvParam;
    m_pRecvParam = Param;
    if (m_pRecvParam)
    {
        m_pRecvParam->AddRef();
    }
    LeaveCriticalSection(&m_csLock);

    if (pOldParam)
    {
        pOldParam->Release();
    }
}

VOID CQUICChannel::RegisterEndProcess(_QUICDisconnectedProcess Process, CBaseObject* Param)
{
    CBaseObject* pOldParam = NULL;

    EnterCriticalSection(&m_csLock);
    m_pfnEndFunc = Process;

    pOldParam = m_pEndParam;
    m_pEndParam = Param;
    if (m_pEndParam)
    {
        m_pEndParam->AddRef();
    }
    LeaveCriticalSection(&m_csLock);

    if (pOldParam)
    {
        pOldParam->Release();
    }
}