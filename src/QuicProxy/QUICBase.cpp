#include "stdafx.h"
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

    InitializeCriticalSection(&m_csLock);
}

IQUICCommunication::~IQUICCommunication()
{
    DeleteCriticalSection(&m_csLock);
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