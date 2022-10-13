#include "stdafx.h"
#include "QUICClient.h"

CQUICClient::CQUICClient(CHAR* ipaddr, WORD dstPort, const CHAR* Keyword) :
    IQUICBase(),
    IQUICCommunication()
{
    m_wDstPort = dstPort;
    m_szIPAddress = strdup(ipaddr);

    m_stKeyword.Length = strlen(Keyword) - 1;
    m_stKeyword.Buffer = (uint8_t*)Keyword;

    m_hConnection = NULL;
    m_hStream = NULL;

    m_hConnectOK = CreateEvent(NULL, TRUE, FALSE, NULL);
}

CQUICClient::~CQUICClient()
{
    if (m_szIPAddress)
    {
        free(m_szIPAddress);
    }
}

BOOL CQUICClient::Init()
{
    BaseInit(QUICBASE_HIGH_THOUGHPUT);

    LoadConfiguration();
    
    AddRef();
    QUIC_STATUS Status = m_pMsQuic->ConnectionOpen(m_hRegistration, ClientConnectionCallback, this, &m_hConnection);

    if (QUIC_FAILED(Status))
    {
        DBG_ERROR("ConnectionOpen failed, 0x%x!\n", Status);
        Release();
        goto Error;
    }

    DBG_INFO("[conn][%p] Connecting...\n", m_hConnection);

    Status = m_pMsQuic->ConnectionStart(m_hConnection, m_hConfiguration, QUIC_ADDRESS_FAMILY_UNSPEC, m_szIPAddress, m_wDstPort);

    if (QUIC_FAILED(Status))
    {
        DBG_ERROR("ConnectionStart failed, 0x%x!\n", Status);
        goto Error;
    }

    if (WaitForSingleObject(m_hConnectOK, 10000) != WAIT_OBJECT_0)
    {
        DBG_ERROR("Wait for connect time out\n");
        goto Error;
    }

    return TRUE;

Error:
    Done();
    return FALSE;
}

VOID CQUICClient::Done()
{
    if (m_hConnection)
    {
        m_pMsQuic->ConnectionClose(m_hConnection);
    }

    BaseDone();
}

VOID CQUICClient::LoadConfiguration()
{
    QUIC_SETTINGS Settings = { 0 };
    
    Settings.IdleTimeoutMs = 1000;
    Settings.IsSet.IdleTimeoutMs = TRUE;

    QUIC_CREDENTIAL_CONFIG CredConfig;
    memset(&CredConfig, 0, sizeof(CredConfig));
    CredConfig.Type = QUIC_CREDENTIAL_TYPE_NONE;
    CredConfig.Flags = QUIC_CREDENTIAL_FLAG_CLIENT | QUIC_CREDENTIAL_FLAG_NO_CERTIFICATE_VALIDATION;
    
    QUIC_STATUS Status = m_pMsQuic->ConfigurationOpen(m_hRegistration, &m_stKeyword, 1, &Settings, sizeof(Settings), NULL, &m_hConfiguration);
    if (QUIC_FAILED(Status))
    {
        DBG_ERROR("ConfigurationOpen failed, 0x%x!\n", Status);
        return;
    }

    Status = m_pMsQuic->ConfigurationLoadCredential(m_hConfiguration, &CredConfig);

    if (QUIC_FAILED(m_pMsQuic))
    {
        printf("ConfigurationLoadCredential failed, 0x%x!\n", Status);
        return;
    }

    return;
}

void CQUICClient::InitializeConnection(HQUIC Connection)
{
    QUIC_STATUS Status;

    AddRef();
    Status = m_pMsQuic->StreamOpen(Connection, QUIC_STREAM_OPEN_FLAG_NONE, ClientStreamCallback, this, &m_hStream);
    if (QUIC_FAILED(Status))
    {
        DBG_ERROR("StreamOpen failed, 0x%x!\n", Status);
        Release();
        goto Error;
    }

    DBG_INFO("[strm][%p] Starting...\n", m_hStream);

    Status = m_pMsQuic->StreamStart(m_hStream, QUIC_STREAM_START_FLAG_IMMEDIATE);

    if (QUIC_FAILED(Status))
    {
        DBG_ERROR("StreamStart failed, 0x%x!\n", Status);
        m_pMsQuic->StreamClose(m_hStream);
        goto Error;
    }

    SetEvent(m_hConnectOK);

Error:
    if (QUIC_FAILED(Status))
    {
        m_pMsQuic->ConnectionShutdown(Connection, QUIC_CONNECTION_SHUTDOWN_FLAG_NONE, 0);
    }
}

QUIC_STATUS CQUICClient::ClientStreamCallback(
    _In_ HQUIC Stream,
    _In_opt_ void* Context,
    _Inout_ QUIC_STREAM_EVENT* Event
    )
{
    CQUICClient* pClient = (CQUICClient*)Context;

    switch (Event->Type)
    {
        case QUIC_STREAM_EVENT_SEND_COMPLETE:
        {
            QUICSendNode* Node = (QUICSendNode*)(Event->SEND_COMPLETE.ClientContext);
            if (Node->SyncHandle)
            {
                SetEvent(Node->SyncHandle);
            }
            free(Node);
            
            break;
        }
        case QUIC_STREAM_EVENT_RECEIVE:
        {
            if (pClient)
            {
                EnterCriticalSection(&pClient->m_csLock);
                if (pClient->m_pfnRecvFunc)
                {
                    pClient->m_pfnRecvFunc((PBYTE)Event->RECEIVE.Buffers, Event->RECEIVE.TotalBufferLength, pClient, pClient->m_pRecvParam);
                }
                LeaveCriticalSection(&pClient->m_csLock);
            }
            
            break;
        }
        case QUIC_STREAM_EVENT_PEER_SEND_ABORTED:
        {
            DBG_INFO("[strm][%p] Peer aborted\n", Stream);
            break;
        }
        case QUIC_STREAM_EVENT_PEER_SEND_SHUTDOWN:
        {
            DBG_INFO("[strm][%p] Peer shut down\n", Stream);
            break;
        }
        case QUIC_STREAM_EVENT_SHUTDOWN_COMPLETE:
        {
            DBG_INFO("[strm][%p] All done\n", Stream);
            if (!Event->SHUTDOWN_COMPLETE.AppCloseInProgress) {
                if (pClient)
                {
                    EnterCriticalSection(&pClient->m_csLock);

                    if (pClient->m_pfnEndFunc)
                    {
                        pClient->m_pfnEndFunc(pClient, pClient->m_pEndParam);
                    }

                    LeaveCriticalSection(&pClient->m_csLock);

                    pClient->m_pMsQuic->StreamClose(Stream);
                    pClient->Release();
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

QUIC_STATUS CQUICClient::ClientConnectionCallback(
    _In_ HQUIC Connection,
    _In_opt_ void* Context,
    _Inout_ QUIC_CONNECTION_EVENT* Event
    )
{
    CQUICClient* pClient = (CQUICClient*)Context;

    switch (Event->Type)
    {
        case QUIC_CONNECTION_EVENT_CONNECTED:
        {
            DBG_INFO("[conn][%p] Connected\n", Connection);
            if (pClient)
            {
                pClient->InitializeConnection(Connection);
            }
            break;
        }
        case QUIC_CONNECTION_EVENT_SHUTDOWN_INITIATED_BY_TRANSPORT:
        {
            if (Event->SHUTDOWN_INITIATED_BY_TRANSPORT.Status == QUIC_STATUS_CONNECTION_IDLE)
            {
                DBG_INFO("[conn][%p] Successfully shut down on idle.\n", Connection);
            }
            else
            {
                DBG_INFO("[conn][%p] Shut down by transport, 0x%x\n", Connection, Event->SHUTDOWN_INITIATED_BY_TRANSPORT.Status);
            }
            break;
        }
        case QUIC_CONNECTION_EVENT_SHUTDOWN_INITIATED_BY_PEER:
        {
            DBG_INFO("[conn][%p] Shut down by peer, 0x%llu\n", Connection, (unsigned long long)Event->SHUTDOWN_INITIATED_BY_PEER.ErrorCode);
            break;
        }
        case QUIC_CONNECTION_EVENT_SHUTDOWN_COMPLETE:
        {
            if (pClient)
            {
                DBG_INFO("[conn][%p] All done\n", Connection);
                if (!Event->SHUTDOWN_COMPLETE.AppCloseInProgress)
                {
                    pClient->m_pMsQuic->ConnectionClose(Connection);
                }
            }
            break;
        }
        case QUIC_CONNECTION_EVENT_RESUMPTION_TICKET_RECEIVED:
        {
            DBG_INFO("[conn][%p] Resumption ticket received (%u bytes):\n", Connection, Event->RESUMPTION_TICKET_RECEIVED.ResumptionTicketLength);
            for (uint32_t i = 0; i < Event->RESUMPTION_TICKET_RECEIVED.ResumptionTicketLength; i++) {
                DBG_INFO("%.2X", (uint8_t)Event->RESUMPTION_TICKET_RECEIVED.ResumptionTicket[i]);
            }
            DBG_INFO("\n");
            break;
        }
    default:
        break;
    }
    return QUIC_STATUS_SUCCESS;
}

BOOL CQUICClient::SendPacket(PBYTE Data, DWORD Length, HANDLE SyncHandle)
{
    QUICSendNode* Node = (QUICSendNode*)malloc(sizeof(QUICSendNode) + Length);
    
    Node->Packet.Buffer = (uint8_t*)Node + sizeof(QUICSendNode);
    Node->Packet.Length = Length;
    Node->SyncHandle = SyncHandle;

    memcpy(Node->Packet.Buffer, Data, Length);

    QUIC_STATUS Status = m_pMsQuic->StreamSend(m_hStream, &Node->Packet, 1, QUIC_SEND_FLAG_NONE, Node);

    if (QUIC_FAILED(Status))
    {
        DBG_ERROR("StreamSend failed, 0x%x!\n", Status);
        free(Node);
        return FALSE;
    }

    return TRUE;
}