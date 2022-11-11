#include "stdafx.h"
#include "QUICCtrlChannel.h"

CQUICCtrlChannel::CQUICCtrlChannel()
{
    m_hPktRecved = CreateEvent(NULL, TRUE, FALSE, NULL);

    InitializeCriticalSection(&m_csPktLock);
}

CQUICCtrlChannel::~CQUICCtrlChannel()
{
    if (m_hPktRecved)
    {
        CloseHandle(m_hPktRecved);
    }

    DeleteCriticalSection(&m_csPktLock);
}

BOOL CQUICCtrlChannel::Init(const QUIC_API_TABLE* pMsQuic, HQUIC hConnection)
{
    CQUICChannel* channel = new CQUICChannel(pMsQuic, CTRL_CHANNEL_NAME, 0x7FFF);
    if (channel)
    {
        channel->RegisterRecvProcess(CQUICCtrlChannel::QUICtrlProcess, this);
        if (!channel->Init(hConnection))
        {
            channel->Release();
            channel = NULL;
        }
    }

    m_pCtrlStream = channel;

    return (m_pCtrlStream != NULL);
}

BOOL CQUICCtrlChannel::Init(const QUIC_API_TABLE* pMsQuic, HQUIC hStream, HQUIC hConnection)
{
    CQUICChannel* channel = new CQUICChannel(hStream, pMsQuic);

    channel->RegisterRecvProcess(CQUICCtrlChannel::QUICtrlProcess, this);
    if (!channel->Init(hConnection))
    {
        channel->Release();
        channel = NULL;
    }

    m_pCtrlStream = channel;
    
    return (m_pCtrlStream != NULL);
}

void CQUICCtrlChannel::Done()
{
    std::list<QUIC_BUFFER>::iterator Itor;
    
    EnterCriticalSection(&m_csPktLock);

    for (Itor = m_CtrlPktList.begin(); Itor != m_CtrlPktList.end(); Itor++)
    {
        free((*Itor).Buffer);
    }

    m_CtrlPktList.clear();

    LeaveCriticalSection(&m_csPktLock);

    if (m_pCtrlStream)
    {
        m_pCtrlStream->Done();
        m_pCtrlStream->Release();
        m_pCtrlStream = NULL;
    }
}

BOOL CQUICCtrlChannel::QUICtrlProcess(PBYTE Data, DWORD Length, IQUICChannel* quic, CBaseObject* Param)
{
    QUIC_BUFFER Buffer;
    Buffer.Length = Length;
    Buffer.Buffer = (PBYTE)malloc(Length);

    memcpy(Buffer.Buffer, Data, Length);

    CQUICCtrlChannel* pCtrl = dynamic_cast<CQUICCtrlChannel*>(Param);

    if (pCtrl)
    {
        EnterCriticalSection(&pCtrl->m_csPktLock);
        pCtrl->m_CtrlPktList.push_back(Buffer);
        SetEvent(pCtrl->m_hPktRecved);
        LeaveCriticalSection(&pCtrl->m_csPktLock);

        return TRUE;
    }
    else
    {
        free(Buffer.Buffer);
        return FALSE;
    }
}

BOOL CQUICCtrlChannel::SendCtrlPacket(PBYTE Data, DWORD DataLen)
{
    return m_pCtrlStream->SendPacket(Data, DataLen);
}

PBYTE CQUICCtrlChannel::RecvCtrlPacket(DWORD* DataLen, DWORD TimeOut)
{
    DWORD WaitRet = WaitForSingleObject(m_hPktRecved, TimeOut);

    if (WaitRet == WAIT_OBJECT_0)
    {
        PBYTE Ret = NULL;
        *DataLen = 0;
        EnterCriticalSection(&m_csPktLock);

        if (!m_CtrlPktList.empty())
        {
            QUIC_BUFFER Node = m_CtrlPktList.front();
            m_CtrlPktList.pop_front();

            if (m_CtrlPktList.empty())
            {
                ResetEvent(m_hPktRecved);
            }

            Ret = Node.Buffer;
            *DataLen = Node.Length;
        }
        else
        {
            ResetEvent(m_hPktRecved);
        }

        LeaveCriticalSection(&m_csPktLock);

        return Ret;
    }
    else
    {
        *DataLen = 0;
        return NULL;
    }
}

void CQUICCtrlChannel::FreeRecvedCtrlPacket(PBYTE Data)
{
    free(Data);
}


