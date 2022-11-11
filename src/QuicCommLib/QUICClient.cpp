#include "stdafx.h"
#include "QUICClient.h"

CQUICClient::CQUICClient(CHAR* ipaddr, WORD dstPort, const CHAR* Keyword) : CQUICBase(), CQUICLink()
{
    m_wDstPort = dstPort;
    m_szIPAddress = strdup(ipaddr);

    m_stKeyword.Length = strlen(Keyword) - 1;
    m_stKeyword.Buffer = (uint8_t*)Keyword;

    m_hConnection = NULL;

    m_hConnectOK = CreateEvent(NULL, TRUE, FALSE, NULL);
}

CQUICClient::~CQUICClient()
{
    if (m_szIPAddress)
    {
        free(m_szIPAddress);
    }

    if (m_hConnectOK)
    {
        CloseHandle(m_hConnectOK);
    }
}

BOOL CQUICClient::Connect()
{
    BaseInit(QUICBASE_HIGH_THOUGHPUT);

    LoadConfiguration();
    
    AddRef();
    QUIC_STATUS Status = m_pMsQuic->ConnectionOpen(m_hRegistration, ClientConnectionCallback, this, &m_hConnection);

    if (QUIC_FAILED(Status))
    {
        DBG_ERROR(_T("ConnectionOpen failed, 0x%x!\n"), Status);
        Release();
        goto Error;
    }

    DBG_INFO(_T("[conn][%p] Connecting...\n"), m_hConnection);

    Status = m_pMsQuic->ConnectionStart(m_hConnection, m_hConfiguration, QUIC_ADDRESS_FAMILY_INET, m_szIPAddress, m_wDstPort);

    if (QUIC_FAILED(Status))
    {
        DBG_ERROR(_T("ConnectionStart failed, 0x%x!\n"), Status);
        goto Error;
    }

    DBG_INFO(_T("[conn][%p] Connecting Start end\n"), m_hConnection);

    if (WaitForSingleObject(m_hConnectOK, 10000) != WAIT_OBJECT_0)
    {
        DBG_ERROR(_T("Wait for connect time out\n"));
        goto Error;
    }

    EnterCriticalSection(&m_csLock);
    m_pCtrlChannel = new CQUICCtrlChannel();
    m_pCtrlChannel->Init(m_pMsQuic, m_hConnection);
    LeaveCriticalSection(&m_csLock);

    return TRUE;

Error:
    Disconnect();
    return FALSE;
}

VOID CQUICClient::Disconnect()
{
    LinkDone();

    if (m_hConnection)
    {
        m_pMsQuic->ConnectionClose(m_hConnection);
    }

    BaseDone();
}

IQUICChannel* CQUICClient::CreateChannel(CHAR* ChannelName, WORD Priority)
{
    CQUICChannel* channel = new CQUICChannel(m_pMsQuic, ChannelName, Priority);
    if (channel)
    {
        if (channel->Init(m_hConnection))
        {
            channel->AddRef();

            EnterCriticalSection(&m_csLock);
            m_Streams.push_back(channel);
            LeaveCriticalSection(&m_csLock);
        }
        else
        {
            channel->Release();
            channel = NULL;
        }
    }

    return channel;
}

VOID CQUICClient::LoadConfiguration()
{
    QUIC_SETTINGS Settings = { 0 };
   
    Settings.IdleTimeoutMs = 0;
    Settings.IsSet.IdleTimeoutMs = TRUE;

    Settings.KeepAliveIntervalMs = 2000;
    Settings.IsSet.KeepAliveIntervalMs = TRUE;

    Settings.PeerBidiStreamCount = 1024;
    Settings.IsSet.PeerBidiStreamCount = TRUE;

    QUIC_CREDENTIAL_CONFIG CredConfig;
    memset(&CredConfig, 0, sizeof(CredConfig));
    CredConfig.Type = QUIC_CREDENTIAL_TYPE_NONE;
    CredConfig.Flags = QUIC_CREDENTIAL_FLAG_CLIENT | QUIC_CREDENTIAL_FLAG_NO_CERTIFICATE_VALIDATION;
    
    QUIC_STATUS Status = m_pMsQuic->ConfigurationOpen(m_hRegistration, &m_stKeyword, 1, &Settings, sizeof(Settings), NULL, &m_hConfiguration);
    if (QUIC_FAILED(Status))
    {
        DBG_ERROR(_T("ConfigurationOpen failed, 0x%x!\n"), Status);
        return;
    }

    Status = m_pMsQuic->ConfigurationLoadCredential(m_hConfiguration, &CredConfig);

    if (QUIC_FAILED(Status))
    {
        printf("ConfigurationLoadCredential failed, 0x%x!\n", Status);
        return;
    }

    return;
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
            DBG_INFO(_T("[conn][%p] Connected\n"), Connection);
            if (pClient)
            {
                SetEvent(pClient->m_hConnectOK);
            }
            break;
        }
        case QUIC_CONNECTION_EVENT_SHUTDOWN_INITIATED_BY_TRANSPORT:
        {
            if (Event->SHUTDOWN_INITIATED_BY_TRANSPORT.Status == QUIC_STATUS_CONNECTION_IDLE)
            {
                DBG_INFO(_T("[conn][%p] Successfully shut down on idle.\n"), Connection);
            }
            else
            {
                DBG_INFO(_T("[conn][%p] Shut down by transport, Status 0x%x, ErrorCode 0x%llu\n"), Connection, Event->SHUTDOWN_INITIATED_BY_TRANSPORT.Status, (unsigned long long)Event->SHUTDOWN_INITIATED_BY_TRANSPORT.ErrorCode);
            }
            break;
        }
        case QUIC_CONNECTION_EVENT_SHUTDOWN_INITIATED_BY_PEER:
        {
            DBG_INFO(_T("[conn][%p] Shut down by peer, 0x%llu\n"), Connection, (unsigned long long)Event->SHUTDOWN_INITIATED_BY_PEER.ErrorCode);
            break;
        }
        case QUIC_CONNECTION_EVENT_SHUTDOWN_COMPLETE:
        {
            if (pClient)
            {
                DBG_INFO(_T("[conn][%p] All done\n"), Connection);
                if (!Event->SHUTDOWN_COMPLETE.AppCloseInProgress)
                {
                    pClient->m_pMsQuic->ConnectionClose(Connection);
                }
            }
            break;
        }
        case QUIC_CONNECTION_EVENT_RESUMPTION_TICKET_RECEIVED:
        {
            DBG_INFO(_T("[conn][%p] Resumption ticket received (%u bytes):\n"), Connection, Event->RESUMPTION_TICKET_RECEIVED.ResumptionTicketLength);
            break;
        }
    default:
        break;
    }
    return QUIC_STATUS_SUCCESS;
}

