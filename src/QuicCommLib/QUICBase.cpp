#include "stdafx.h"
#include "corelib.h"
#include "QUICBase.h"

IQUICBase::IQUICBase()
{
    m_pMsQuic = NULL;
    m_stRegConfig.AppName = "quic_test";
    m_stRegConfig.ExecutionProfile = QUIC_EXECUTION_PROFILE_LOW_LATENCY;
    m_hRegistration = NULL;
    m_hConfiguration = NULL;
}

IQUICBase::~IQUICBase()
{
    
}

BOOL IQUICBase::BaseInit(QUICBASE_TYPE Type)
{
    QUIC_STATUS Status = MsQuicOpen2(&m_pMsQuic);

    if (QUIC_FAILED(Status))
    {
        DBG_ERROR(_T("MsQuicOpen2 failed, 0x%x!\n"), Status);
        goto Error;
    }

    m_stRegConfig.ExecutionProfile = (QUIC_EXECUTION_PROFILE)Type;

    Status = m_pMsQuic->RegistrationOpen(&m_stRegConfig, &m_hRegistration);

    if (QUIC_FAILED(Status))
    {
        DBG_ERROR(_T("RegistrationOpen failed, 0x%x!\n"), Status);
        goto Error;
    }

    return TRUE;

Error:
    BaseDone();
    return FALSE;
}

VOID IQUICBase::BaseDone()
{
    if (m_pMsQuic != NULL)
    {
        if (m_hConfiguration != NULL)
        {
            m_pMsQuic->ConfigurationClose(m_hConfiguration);
        }

        if (m_hRegistration != NULL)
        {
            m_pMsQuic->RegistrationClose(m_hRegistration);
        }

        MsQuicClose(m_pMsQuic);
    }
}

IQUICCommunication::IQUICCommunication()
{
    m_pfnRecvFunc = NULL;
    m_pRecvParam = NULL;

    m_pfnEndFunc = NULL;
    m_pEndParam = NULL;

    m_hDataBufferStream = CreateDataStreamBuffer();

    InitializeCriticalSection(&m_csLock);
}

IQUICCommunication::~IQUICCommunication()
{
    CloseDataStreamBuffer(m_hDataBufferStream);
    DeleteCriticalSection(&m_csLock);
}

void IQUICCommunication::ProcessRecvEvent(QUIC_STREAM_EVENT* RecvEvent)
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
            EnterCriticalSection(&m_csLock);
            if (m_pfnRecvFunc)
            {
                m_pfnRecvFunc(pkt->Data, pkt->Length - sizeof(QUICPkt), this, m_pRecvParam);
            }
            LeaveCriticalSection(&m_csLock);
            free(pkt);
        }
        else
        {
            break;
        }
    }

    return;
}

void IQUICCommunication::ProcessShutdownEvent(QUIC_STREAM_EVENT* ShutdownEvent)
{
    EnterCriticalSection(&m_csLock);

    if (m_pfnEndFunc)
    {
        m_pfnEndFunc(this, m_pEndParam);
    }

    LeaveCriticalSection(&m_csLock);

    return;
}

void IQUICCommunication::ProcessSendCompleteEvent(QUIC_STREAM_EVENT* SendCompleteEvent)
{
    QUICSendNode* Node = (QUICSendNode*)(SendCompleteEvent->SEND_COMPLETE.ClientContext);
    
    if (Node->SyncHandle)
    {
        SetEvent(Node->SyncHandle);
    }

    free(Node);

    return;
}

QUICSendNode* IQUICCommunication::CreateQUICSendNode(PBYTE Data, DWORD Length, HANDLE SyncHandle)
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

void IQUICCommunication::AddQuicBufferToDataBuffer(const QUIC_BUFFER* quicBuffer)
{
    DataStreamBufferAddData(m_hDataBufferStream, quicBuffer->Buffer, quicBuffer->Length);
    return;
}

QUICPkt* IQUICCommunication::GetQuicPktFromDataBuffer()
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

VOID IQUICCommunication::RegisterRecvProcess(_QUICRecvPacketProcess Process, CBaseObject* Param)
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

VOID IQUICCommunication::RegisterEndProcess(_QUICDisconnectedProcess Process, CBaseObject* Param)
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