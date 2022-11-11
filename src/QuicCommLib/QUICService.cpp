#include "stdafx.h"
#include "QUICService.h"

CQUICService::CQUICService(WORD Port, const CHAR* Keyword, const CHAR* CertFilePath, const CHAR* KeyFilePath)
    : CQUICBase()
{
    m_wUdpPort = Port;
    m_hListener = NULL;

    m_stKeyword.Length = strlen(Keyword) - 1;
    m_stKeyword.Buffer = (uint8_t*)Keyword;

    m_szCertFilePath = strdup(CertFilePath);
    m_szKeyFilePath = strdup(KeyFilePath);

    m_hAcceptEvent = CreateEvent(NULL, TRUE, FALSE, NULL);

    m_hStopEvent = CreateEvent(NULL, TRUE, FALSE, NULL);

    InitializeCriticalSection(&m_csLock);
}

CQUICService::~CQUICService()
{
    free(m_szCertFilePath);
    free(m_szKeyFilePath);

    if (m_hAcceptEvent)
    {
        CloseHandle(m_hAcceptEvent);
    }

    if (m_hStopEvent)
    {
        CloseHandle(m_hStopEvent);
    }

    DeleteCriticalSection(&m_csLock);
}

void CQUICService::LoadConfiguration()
{
    QUIC_SETTINGS Settings = { 0 };

    Settings.IdleTimeoutMs = 0;
    Settings.IsSet.IdleTimeoutMs = TRUE;

    Settings.KeepAliveIntervalMs = 2000;
    Settings.IsSet.KeepAliveIntervalMs = TRUE;

    Settings.ServerResumptionLevel = QUIC_SERVER_RESUME_AND_ZERORTT;
    Settings.IsSet.ServerResumptionLevel = TRUE;

    Settings.PeerBidiStreamCount = 1024;
    Settings.IsSet.PeerBidiStreamCount = TRUE;

    QUIC_STATUS Status = m_pMsQuic->ConfigurationOpen(m_hRegistration, &m_stKeyword, 1, &Settings, sizeof(Settings), NULL, &m_hConfiguration);
    if (QUIC_FAILED(Status))
    {
        DBG_ERROR(_T("ConfigurationOpen failed, 0x%x!\n"), Status);
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
        DBG_ERROR(_T("ConfigurationLoadCredential failed, 0x%x!\n"), Status);
        return;
    }

    return;
}

BOOL CQUICService::StartListen()
{
    ResetEvent(m_hStopEvent);

    BaseInit(QUICBASE_HIGH_THOUGHPUT);

    QUIC_ADDR Address = { 0 };
    QuicAddrSetFamily(&Address, QUIC_ADDRESS_FAMILY_INET);
    QuicAddrSetPort(&Address, m_wUdpPort);

    LoadConfiguration();

    AddRef();
    QUIC_STATUS Status = m_pMsQuic->ListenerOpen(m_hRegistration, CQUICService::ServerListenerCallback, this, &m_hListener);

    if (QUIC_FAILED(Status))
    {
        DBG_ERROR(_T("ListenerOpen failed, 0x%x!\n"), Status);
        goto Error;
    }

    Status = m_pMsQuic->ListenerStart(m_hListener, &m_stKeyword, 1, &Address);

    if (QUIC_FAILED(Status))
    {
        DBG_ERROR(_T("ListenerStart failed, 0x%x!\n"), Status);
        goto Error;
    }

    DBG_ERROR(_T("QUIC Listener Started\n"));

    return TRUE;

Error:
    StopListen();
    return FALSE;
}

VOID CQUICService::StopListen()
{
    SetEvent(m_hStopEvent);

    if (m_hListener != NULL)
    {
        m_pMsQuic->ListenerClose(m_hListener);
    }

    BaseDone();
}

IQUICServer* CQUICService::Accept(HANDLE StopEvent)
{
    HANDLE h[3] = {
        m_hAcceptEvent,
        m_hStopEvent,
        StopEvent
    };

    DWORD WaitRet = WaitForMultipleObjects(3, h, FALSE, INFINITE);
    if (WaitRet == WAIT_OBJECT_0)
    {
        CQUICServer* Ret = NULL;
        EnterCriticalSection(&m_csLock);

        if (!m_ServerList.empty())
        {
            CQUICServer* pServer = m_ServerList.front();
            m_ServerList.pop_front();

            if (m_ServerList.empty())
            {
                ResetEvent(m_hAcceptEvent);
            }

            if (pServer->WaitForServerReady(StopEvent))
            {
                Ret = pServer;
            }
            else
            {
                pServer->Disconnect();
                pServer->Release();
            }
            
        }
        else
        {
            ResetEvent(m_hAcceptEvent);
        }

        LeaveCriticalSection(&m_csLock);

        return Ret;
    }
    else
    {
        return NULL;
    }
}

CQUICServer* CQUICService::CreateQUICServer(HQUIC hConnection)
{
    CQUICServer* pServer = new CQUICServer(hConnection, m_pMsQuic, this);

    if (!pServer->Init(m_hConfiguration))
    {
        pServer->Release();
        pServer = NULL;
    }

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
                CQUICServer* pServer = pService->CreateQUICServer(Event->NEW_CONNECTION.Connection);

                if (pServer)
                {
                    EnterCriticalSection(&pService->m_csLock);

                    pService->m_ServerList.push_back(pServer);

                    SetEvent(pService->m_hAcceptEvent);

                    LeaveCriticalSection(&pService->m_csLock);

                    Status = QUIC_STATUS_SUCCESS;
                }
            }
            break;
        }
        case QUIC_LISTENER_EVENT_STOP_COMPLETE:
        {
            if (pService)
            {
                pService->Release();
            }
            Status = QUIC_STATUS_SUCCESS;
            break;
        }
        default:
        {
            break;
        }
    }
    return Status;
}

