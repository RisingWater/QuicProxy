#include "stdafx.h"
#include "corelib.h"
#include "QUICBase.h"
#include "QUICCtrlChannel.h"

CQUICBase::CQUICBase()
{
    m_pMsQuic = NULL;
    m_stRegConfig.AppName = "quic_test";
    m_stRegConfig.ExecutionProfile = QUIC_EXECUTION_PROFILE_LOW_LATENCY;
    m_hRegistration = NULL;
    m_hConfiguration = NULL;
}

CQUICBase::~CQUICBase()
{
    
}

BOOL CQUICBase::BaseInit(QUICBASE_TYPE Type)
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

VOID CQUICBase::BaseDone()
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

BOOL CQUICLink::SendCtrlPacket(PBYTE Data, DWORD DataLen)
{
    BOOL Ret = FALSE;
    CQUICCtrlChannel* channel = NULL;

    EnterCriticalSection(&m_csLock);
    if (m_pCtrlChannel)
    {
        channel = m_pCtrlChannel;
        channel->AddRef();
    }
    LeaveCriticalSection(&m_csLock);

    if (channel)
    {
        Ret = channel->SendCtrlPacket(Data, DataLen);
        channel->Release();
    }

    return Ret;
}

CQUICLink::CQUICLink()
{
    m_pCtrlChannel = NULL;

    InitializeCriticalSection(&m_csLock);
}

CQUICLink::~CQUICLink()
{
    DeleteCriticalSection(&m_csLock);
}

void CQUICLink::LinkDone()
{
    std::list<CQUICChannel*>::iterator Itor;
    EnterCriticalSection(&m_csLock);

    for (Itor = m_Streams.begin(); Itor != m_Streams.end(); Itor++)
    {
        CQUICChannel* channel = (*Itor);
        channel->Done();
        channel->Release();
    }

    m_Streams.clear();

    if (m_pCtrlChannel)
    {
        m_pCtrlChannel->Done();
        m_pCtrlChannel->Release();
        m_pCtrlChannel = NULL;
    }

    LeaveCriticalSection(&m_csLock);
}

PBYTE CQUICLink::RecvCtrlPacket(DWORD* DataLen, DWORD TimeOut)
{
    PBYTE Ret = NULL;
    CQUICCtrlChannel* channel = NULL;

    EnterCriticalSection(&m_csLock);
    if (m_pCtrlChannel)
    {
        channel = m_pCtrlChannel;
        channel->AddRef();
    }
    LeaveCriticalSection(&m_csLock);

    if (channel)
    {
        Ret = channel->RecvCtrlPacket(DataLen, TimeOut);
        channel->Release();
    }

    return Ret;
}

void CQUICLink::FreeRecvedCtrlPacket(PBYTE Data)
{
    CQUICCtrlChannel* channel = NULL;

    EnterCriticalSection(&m_csLock);
    if (m_pCtrlChannel)
    {
        channel = m_pCtrlChannel;
        channel->AddRef();
    }
    LeaveCriticalSection(&m_csLock);

    if (channel)
    {
        channel->FreeRecvedCtrlPacket(Data);
        channel->Release();
    }

    return;
}

void CQUICLink::DestoryChannel(IQUICChannel* pChannel)
{
    std::list<CQUICChannel*>::iterator Itor;
    EnterCriticalSection(&m_csLock);

    for (Itor = m_Streams.begin(); Itor != m_Streams.end(); Itor++)
    {
        CQUICChannel* channel = (*Itor);
        if (strcmp(channel->GetChannelName(), pChannel->GetChannelName()) == 0)
        {
            channel->Done();
            channel->Release();
            m_Streams.erase(Itor);
            break;
        }
    }

    LeaveCriticalSection(&m_csLock);
}