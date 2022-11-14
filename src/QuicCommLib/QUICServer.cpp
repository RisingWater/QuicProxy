#include "stdafx.h"
#include "QUICServer.h"
#include "QUICService.h"

CQUICServer::CQUICServer(HQUIC hConnection, const QUIC_API_TABLE* pMsQuic, CQUICService* pService) : CQUICLink()
{
    m_hConnection = hConnection;
    m_pService = pService;
    m_pMsQuic = pMsQuic;
    if (m_pService)
    {
        m_pService->AddRef();
    }

    m_pCtrlChannel = NULL;

    m_hCtrlChannelReady = CreateEvent(NULL, TRUE, FALSE, NULL);

    InitializeCriticalSection(&m_csLock);
}

CQUICServer::~CQUICServer()
{
    if (m_pService)
    {
        m_pService->Release();
    }

    if (m_hCtrlChannelReady)
    {
        CloseHandle(m_hCtrlChannelReady);
    }

    DeleteCriticalSection(&m_csLock);
}

BOOL CQUICServer::Init(HQUIC hConfiguration)
{
    AddRef();
    m_pMsQuic->SetCallbackHandler(m_hConnection, (void*)ServerConnectionCallback, this);

    QUIC_STATUS Status = m_pMsQuic->ConnectionSetConfiguration(m_hConnection, hConfiguration);
    if (QUIC_FAILED(Status))
    {
        m_pMsQuic->ConnectionShutdown(m_hConnection, QUIC_CONNECTION_SHUTDOWN_FLAG_NONE, 0);
        return FALSE;
    }

    return TRUE;
}

void CQUICServer::Disconnect()
{
    LinkDone();

    if (m_hConnection)
    {
        m_pMsQuic->ConnectionShutdown(m_hConnection, QUIC_CONNECTION_SHUTDOWN_FLAG_NONE, 0);
    }

    return;
}

BOOL CQUICServer::WaitForServerReady(HANDLE hStopEvent)
{
    HANDLE h[2] = {
        m_hCtrlChannelReady,
        hStopEvent
    };

    DWORD Ret = WaitForMultipleObjects(2, h, FALSE, INFINITE);

    return Ret == WAIT_OBJECT_0;
}

QUIC_STATUS CQUICServer::ServerConnectionCallback(
    _In_ HQUIC Connection,
    _In_opt_ void* Context,
    _Inout_ QUIC_CONNECTION_EVENT* Event
    )
{
    CQUICServer* pServer = (CQUICServer*)Context;

    switch (Event->Type)
    {
    case QUIC_CONNECTION_EVENT_CONNECTED:
    {
        DBG_INFO(_T("[conn][%p] Connected\n"), Connection);
        if (pServer)
        {
            pServer->m_pMsQuic->ConnectionSendResumptionTicket(Connection, QUIC_SEND_RESUMPTION_FLAG_NONE, 0, NULL);
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
            DBG_INFO(_T("[conn][%p] Shut down by transport, 0x%x\n"), Connection, Event->SHUTDOWN_INITIATED_BY_TRANSPORT.Status);
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
        DBG_INFO(_T("[conn][%p] All done\n"), Connection);
        if (pServer)
        {
            pServer->m_pMsQuic->ConnectionClose(Connection);
            pServer->LinkDisconnectNotify();
            pServer->Release();
        }
        break;
    }
    case QUIC_CONNECTION_EVENT_PEER_STREAM_STARTED:
    {
        if (pServer)
        {
            DBG_INFO(_T("[strm][%p] Peer started\n"), Event->PEER_STREAM_STARTED.Stream);

            EnterCriticalSection(&pServer->m_csLock);
            if (pServer->m_pCtrlChannel == NULL)
            {
                pServer->m_pCtrlChannel = new CQUICCtrlChannel();
                if (!pServer->m_pCtrlChannel->Init(pServer->m_pMsQuic, Event->PEER_STREAM_STARTED.Stream, Connection))
                {
                    pServer->m_pCtrlChannel->Done();
                    pServer->m_pCtrlChannel->Release();
                    pServer->m_pCtrlChannel = NULL;
                }

                SetEvent(pServer->m_hCtrlChannelReady);
            }
            else
            {
                pServer->CreateChannel(Event->PEER_STREAM_STARTED.Stream);
            }

            LeaveCriticalSection(&pServer->m_csLock);
        }
        break;
    }
    case QUIC_CONNECTION_EVENT_RESUMED:
    {
        DBG_INFO(_T("[conn][%p] Connection resumed!\n"), Connection);
        break;
    }
    default:
        break;
    }

    return QUIC_STATUS_SUCCESS;
}

void CQUICServer::CreateChannel(HQUIC hStream)
{
    CQUICChannel* channel = new CQUICChannel(hStream, m_pMsQuic);
    if (!channel->Init(m_hConnection))
    {
        channel->Release();
        return;
    }

    EnterCriticalSection(&m_csLock);
    m_Streams.push_back(channel);
    LeaveCriticalSection(&m_csLock);
}

IQUICChannel* CQUICServer::FindChannelByName(const CHAR* ChannelName)
{
    std::list<CQUICChannel*>::iterator Itor;
    IQUICChannel* Ret = NULL;

    EnterCriticalSection(&m_csLock);

    for (Itor = m_Streams.begin(); Itor != m_Streams.end(); Itor++)
    {
        if (strcmp(ChannelName, (*Itor)->GetChannelName()) == 0)
        {
            Ret = (*Itor);
            Ret->AddRef();
            break;
        }
    }

    LeaveCriticalSection(&m_csLock);

    return Ret;
}

IQUICChannel* CQUICServer::WaitForChannelReady(const CHAR* channelName, HANDLE hStopEvent, DWORD TimeOut)
{
    DWORD WaitTime = 0;
    while (TRUE)
    {
        IQUICChannel* Ret = FindChannelByName(channelName);

        if (Ret)
        {
            return Ret;
        }

        DWORD WaitRet = WaitForSingleObject(hStopEvent, 100);
        if (WaitRet != WAIT_TIMEOUT)
        {
            break;
        }

        WaitTime += 100;
        if (WaitTime > TimeOut)
        {
            break;
        }
    }

    return NULL;
}
