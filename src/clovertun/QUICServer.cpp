#include "stdafx.h"
#include "QUICServer.h"

CQUICService::CQUICService(WORD Port, const CHAR* Keyword, CHAR* CertFilePath, CHAR* KeyFilePath) 
    : IQUICBase()
{
    m_wUdpPort = Port;
    m_hListener = NULL;

    m_stKeyword.Length = strlen(Keyword) - 1;
    m_stKeyword.Buffer = (uint8_t*)Keyword;

    m_szCertFilePath = strdup(CertFilePath);
    m_szKeyFilePath = strdup(KeyFilePath);

    m_pfnAcceptFunc = NULL;
    m_pAcceptParam = NULL;

    InitializeCriticalSection(&m_csLock);
}

CQUICService::~CQUICService()
{
    free(m_szCertFilePath);
    free(m_szKeyFilePath);

    DeleteCriticalSection(&m_csLock);
}

void CQUICService::LoadConfiguration()
{
    QUIC_SETTINGS Settings = { 0 };

    Settings.IdleTimeoutMs = 1000ULL;
    Settings.IsSet.IdleTimeoutMs = TRUE;
    
    Settings.ServerResumptionLevel = QUIC_SERVER_RESUME_AND_ZERORTT;
    Settings.IsSet.ServerResumptionLevel = TRUE;
    
    Settings.PeerBidiStreamCount = 1;
    Settings.IsSet.PeerBidiStreamCount = TRUE;

    QUIC_STATUS Status = m_pMsQuic->ConfigurationOpen(m_hRegistration, &m_stKeyword, 1, &Settings, sizeof(Settings), NULL, &m_hConfiguration);
    if (QUIC_FAILED(Status))
    {
        DBG_ERROR("ConfigurationOpen failed, 0x%x!\n", Status);
        return;
    }

    QUIC_CREDENTIAL_CONFIG CredConfig;
    QUIC_CERTIFICATE_FILE  CertFile;

    memset(&CredConfig, 0, sizeof(QUIC_CREDENTIAL_CONFIG));

    CertFile.CertificateFile = m_szCertFilePath;
    CertFile.PrivateKeyFile = m_szKeyFilePath;
    CredConfig.Type = QUIC_CREDENTIAL_TYPE_CERTIFICATE_FILE;
    CredConfig.CertificateFile = &CertFile;

    Status = m_pMsQuic->ConfigurationLoadCredential(m_hConfiguration, &CredConfig);

    if (QUIC_FAILED(Status))
    {
        DBG_ERROR("ConfigurationLoadCredential failed, 0x%x!\n", Status);
        return;
    }

    return;
}

BOOL CQUICService::Init()
{
    BaseInit(QUICBASE_HIGH_THOUGHPUT);

    QUIC_ADDR Address = { 0 };
    QuicAddrSetFamily(&Address, QUIC_ADDRESS_FAMILY_INET);
    QuicAddrSetPort(&Address, m_wUdpPort);

    LoadConfiguration();

    AddRef();
    QUIC_STATUS Status = m_pMsQuic->ListenerOpen(m_hRegistration, CQUICService::ServerListenerCallback, this, &m_hListener);

    if (QUIC_FAILED(Status))
    {
        DBG_ERROR("ListenerOpen failed, 0x%x!\n", Status);
        goto Error;
    }

    Status = m_pMsQuic->ListenerStart(m_hListener, &m_stKeyword, 1, &Address);

    if (QUIC_FAILED(Status))
    {
        DBG_ERROR("ListenerStart failed, 0x%x!\n", Status);
        goto Error;
    }

    return TRUE;

Error:
    Done();
    return FALSE;
}

VOID CQUICService::Done()
{
    if (m_hListener != NULL)
    {
        m_pMsQuic->ListenerClose(m_hListener);
    }

    BaseDone();
}

CQUICServer* CQUICService::CreateQUICServer(HQUIC Stream)
{
    CQUICServer* pServer = new CQUICServer(Stream, this);
    return pServer;
}

QUIC_STATUS CQUICService::ServerListenerCallback(
    _In_ HQUIC Listener,
    _In_opt_ void* Context,
    _Inout_ QUIC_LISTENER_EVENT* Event
    )
{
    Listener;

    CQUICService* pService = (CQUICService*)Context;

    QUIC_STATUS Status = QUIC_STATUS_NOT_SUPPORTED;
    switch (Event->Type)
    {
        case QUIC_LISTENER_EVENT_NEW_CONNECTION:
        {
            if (pService)
            {
                pService->AddRef();
                pService->m_pMsQuic->SetCallbackHandler(Event->NEW_CONNECTION.Connection, (void*)ServerConnectionCallback, pService);
                Status = pService->m_pMsQuic->ConnectionSetConfiguration(Event->NEW_CONNECTION.Connection, pService->m_hConfiguration);
            }
            break;
        }
        case QUIC_LISTENER_EVENT_STOP_COMPLETE:
        {
            if (pService)
            {
                pService->Release();
            }
            break;
        }
        default:
        {
            break;
        }
    }
    return Status;
}

QUIC_STATUS CQUICService::ServerConnectionCallback(
    _In_ HQUIC Connection,
    _In_opt_ void* Context,
    _Inout_ QUIC_CONNECTION_EVENT* Event
)
{
    CQUICService* pService = (CQUICService*)Context;

    switch (Event->Type)
    {
        case QUIC_CONNECTION_EVENT_CONNECTED:
        {
            DBG_INFO("[conn][%p] Connected\n", Connection);
            if (pService)
            {
                pService->m_pMsQuic->ConnectionSendResumptionTicket(Connection, QUIC_SEND_RESUMPTION_FLAG_NONE, 0, NULL);
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
            DBG_INFO("[conn][%p] All done\n", Connection);
            if (pService)
            {
                pService->m_pMsQuic->ConnectionClose(Connection);
                pService->Release();
            }
            break;
        }
        case QUIC_CONNECTION_EVENT_PEER_STREAM_STARTED:
        {
            if (pService)
            {
                DBG_INFO("[strm][%p] Peer started\n", Event->PEER_STREAM_STARTED.Stream);

                EnterCriticalSection(&pService->m_csLock);

                CQUICServer* pServer = pService->CreateQUICServer(Event->PEER_STREAM_STARTED.Stream);

                pService->m_ServerMap[Event->PEER_STREAM_STARTED.Stream] = pServer;

                if (pService->m_pfnAcceptFunc)
                {
                    pService->m_pfnAcceptFunc(pServer, pService->m_pAcceptParam);
                }

                LeaveCriticalSection(&pService->m_csLock);

                pService->m_pMsQuic->SetCallbackHandler(Event->PEER_STREAM_STARTED.Stream, (void*)CQUICServer::ServerStreamCallback, pServer);
            }
            break;
        }
        case QUIC_CONNECTION_EVENT_RESUMED:
        {
            DBG_INFO("[conn][%p] Connection resumed!\n", Connection);
            break;
        }
        default:
            break;
    }

    return QUIC_STATUS_SUCCESS;
}

void CQUICService::ServerDisconnect(HQUIC Stream)
{
    std::map<HQUIC, CQUICServer*>::iterator Itor;

    EnterCriticalSection(&m_csLock);
    Itor = m_ServerMap.find(Stream);
    if (Itor != m_ServerMap.end())
    {
        m_ServerMap.erase(Itor);
    }

    LeaveCriticalSection(&m_csLock);
}

VOID CQUICService::RegisterConnectedProcess(_ClientConnectedProcess Process, CBaseObject* pParam)
{
    CBaseObject* pOldParam = NULL;

    EnterCriticalSection(&m_csLock);
    m_pfnAcceptFunc = Process;

    pOldParam = m_pAcceptParam;
    m_pAcceptParam = pParam;
    if (m_pAcceptParam)
    {
        m_pAcceptParam->AddRef();
    }
    LeaveCriticalSection(&m_csLock);

    if (pOldParam)
    {
        pOldParam->Release();
    }
}

CQUICServer::CQUICServer(HQUIC Stream, CQUICService* pService) :
    IQUICCommunication(),
    CBaseObject()
{
    m_hStream = Stream;
    m_pService = pService;
    if (m_pService)
    {
        m_pService->AddRef();
    }
}

CQUICServer::~CQUICServer()
{
    if (m_pService)
    {
        m_pService->Release();
    }
}

void CQUICServer::CloseConnection()
{
    void* SendBufferRaw = malloc(sizeof(QUIC_BUFFER) + 4);
    if (SendBufferRaw == NULL)
    {
        DBG_ERROR("SendBuffer allocation failed!\n");
        m_pService->m_pMsQuic->StreamShutdown(m_hStream, QUIC_STREAM_SHUTDOWN_FLAG_ABORT, 0);
        return;
    }
    QUIC_BUFFER* SendBuffer = (QUIC_BUFFER*)SendBufferRaw;
    SendBuffer->Buffer = (uint8_t*)SendBufferRaw + sizeof(QUIC_BUFFER);
    SendBuffer->Length = 4;

    DBG_INFO("[strm][%p] Sending CloseConnection...\n", m_hStream);

    QUIC_STATUS Status = m_pService->m_pMsQuic->StreamSend(m_hStream, SendBuffer, 1, QUIC_SEND_FLAG_FIN, SendBuffer);
    if (QUIC_FAILED(Status))
    {
        DBG_ERROR("StreamSend failed, 0x%x!\n", Status);
        free(SendBufferRaw);
        m_pService->m_pMsQuic->StreamShutdown(m_hStream, QUIC_STREAM_SHUTDOWN_FLAG_ABORT, 0);
    }
}

QUIC_STATUS QUIC_API CQUICServer::ServerStreamCallback(
    _In_ HQUIC Stream,
    _In_opt_ void* Context,
    _Inout_ QUIC_STREAM_EVENT* Event
    )
{
    CQUICServer* pServer = (CQUICServer*)Context;

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
            if (pServer)
            {
                EnterCriticalSection(&pServer->m_csLock);
                if (pServer->m_pfnRecvFunc)
                {
                    pServer->m_pfnRecvFunc((PBYTE)Event->RECEIVE.Buffers, (DWORD)Event->RECEIVE.TotalBufferLength, pServer, pServer->m_pRecvParam);
                }
                LeaveCriticalSection(&pServer->m_csLock);
            }

            break;
        }
        case QUIC_STREAM_EVENT_PEER_SEND_SHUTDOWN:
        {
            if (pServer)
            {
                DBG_INFO("[strm][%p] Peer shut down\n", Stream);
                pServer->CloseConnection();
            }
            break;
        }
        case QUIC_STREAM_EVENT_PEER_SEND_ABORTED:
        {
            DBG_INFO("[strm][%p] Peer aborted\n", Stream);
            if (pServer)
            {
                pServer->m_pService->m_pMsQuic->StreamShutdown(Stream, QUIC_STREAM_SHUTDOWN_FLAG_ABORT, 0);
            }
            break;
        }
        case QUIC_STREAM_EVENT_SHUTDOWN_COMPLETE:
        {
            DBG_INFO("[strm][%p] All done\n", Stream);

            if (pServer)
            {
                pServer->m_pService->ServerDisconnect(Stream);
                pServer->m_pService->m_pMsQuic->StreamClose(Stream);
                pServer->Release();
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

BOOL CQUICServer::SendPacket(PBYTE Data, DWORD Length, HANDLE SyncHandle)
{
    QUICSendNode* Node = (QUICSendNode*)malloc(sizeof(QUICSendNode) + Length);

    Node->Packet.Buffer = (uint8_t*)Node + sizeof(QUICSendNode);
    Node->Packet.Length = Length;
    Node->SyncHandle = SyncHandle;

    memcpy(Node->Packet.Buffer, Data, Length);

    QUIC_STATUS Status = m_pService->m_pMsQuic->StreamSend(m_hStream, &Node->Packet, 1, QUIC_SEND_FLAG_NONE, Node);

    if (QUIC_FAILED(Status))
    {
        DBG_ERROR("StreamSend failed, 0x%x!\n", Status);
        free(Node);
        return FALSE;
    }

    return TRUE;
}