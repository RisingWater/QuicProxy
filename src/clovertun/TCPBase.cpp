#include "stdafx.h"
#include "TCPBase.h"
#include "TCPCommon.h"

CTCPBase::CTCPBase()
{
	m_hDataStreamBuffer = CreateDataStreamBuffer();

	m_hStopEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
	m_hSendEvent = CreateEvent(NULL, TRUE, FALSE, NULL);

	m_hRecvThread = NULL;
	m_hSendThread = NULL;

    m_pfnRecvFunc = NULL;
    m_pRecvParam = NULL;
    m_pfnEndFunc = NULL;
    m_pEndParam = NULL;

    m_dwSendBufferSize = 0;

	InitializeCriticalSection(&m_csSendLock);
	InitializeCriticalSection(&m_csFunc);
}

CTCPBase::~CTCPBase()
{
	if (m_hStopEvent)
	{
		CloseHandle(m_hStopEvent);
	}
	if (m_hSendEvent)
	{
		CloseHandle(m_hSendEvent);
	}
	if (m_hRecvThread)
	{
		CloseHandle(m_hRecvThread);
	}
	if (m_hSendThread)
	{
		CloseHandle(m_hSendThread);
	}

	if (m_hDataStreamBuffer)
	{
		CloseDataStreamBuffer(m_hDataStreamBuffer);
        m_hDataStreamBuffer = NULL;
	}

    DeleteCriticalSection(&m_csSendLock);
	DeleteCriticalSection(&m_csFunc);
}

BOOL CTCPBase::InitBase()
{
	int flag = 1;
	setsockopt(m_hSock, IPPROTO_TCP, TCP_NODELAY, (char *)&flag, sizeof(int));
    
    flag = 8 * 1024 * 1024;
    setsockopt(m_hSock, SOL_SOCKET, SO_RCVBUF, (char *)&flag, sizeof(int));
    setsockopt(m_hSock, SOL_SOCKET, SO_SNDBUF, (char *)&flag, sizeof(int));

#ifdef WIN32
	flag = 1;
	ioctlsocket(m_hSock, FIONBIO, (unsigned long *)&flag);
#else
	ioctl(m_hSock, FIONBIO, 0);
#endif

    AddRef();
	m_hRecvThread = CreateThread(NULL, 0, CTCPBase::RecvProc, this, 0, NULL);

    AddRef();
	m_hSendThread = CreateThread(NULL, 0, CTCPBase::SendProc, this, 0, NULL);

    return TRUE;
}

DWORD CTCPBase::GetSendBuffSize()
{
    DWORD len;
    EnterCriticalSection(&m_csSendLock);

    len = m_dwSendBufferSize;

    LeaveCriticalSection(&m_csSendLock);

    return len;
}

VOID CTCPBase::DoneBase()
{
	SetEvent(m_hStopEvent);
}

VOID CTCPBase::SendPacket(BASE_PACKET_T* Packet, HANDLE SyncHandle)
{
    TCPSendNode Node;
    Node.Packet = Packet;
    Node.SyncHandle = SyncHandle;

	EnterCriticalSection(&m_csSendLock);
    m_SendList.push_back(Node);
    m_dwSendBufferSize += Node.Packet->Length;
    SetEvent(m_hSendEvent);
	LeaveCriticalSection(&m_csSendLock);
}

VOID CTCPBase::RegisterRecvProcess(_TCPRecvPacketProcess Process, CBaseObject* Param)
{
    CBaseObject* pOldParam = NULL;

    EnterCriticalSection(&m_csFunc);
	m_pfnRecvFunc = Process;

    pOldParam = m_pRecvParam;
    m_pRecvParam = Param;
    if (m_pRecvParam)
    {
        m_pRecvParam->AddRef();
    }
    LeaveCriticalSection(&m_csFunc);

    if (pOldParam)
    {
        pOldParam->Release();
    }       
}

VOID CTCPBase::RegisterEndProcess(_TCPEndProcess Process, CBaseObject* Param)
{
    CBaseObject* pOldParam = NULL;

    EnterCriticalSection(&m_csFunc);
	m_pfnEndFunc = Process;

    pOldParam = m_pEndParam;
    m_pEndParam = Param;
    if (m_pEndParam)
    {
        m_pEndParam->AddRef();
    }
    LeaveCriticalSection(&m_csFunc);

    if (pOldParam)
    {
        pOldParam->Release();
    }       
}

DWORD WINAPI CTCPBase::RecvProc(void* pParam)
{
	CTCPBase* tcp = (CTCPBase*)pParam;
	while (TRUE)
	{
		if (!tcp->RecvProcess(tcp->m_hStopEvent))
		{
			break;
		}
	}

    EnterCriticalSection(&tcp->m_csFunc);
    if (tcp->m_pfnEndFunc)
    {
        tcp->m_pfnEndFunc(tcp, tcp->m_pEndParam);
    }
    LeaveCriticalSection(&tcp->m_csFunc);

    DBG_INFO(_T("CTCPBase: Recv Thread Stop\r\n"));

    SetEvent(tcp->m_hStopEvent);

	tcp->Release();

	return 0;
}

DWORD WINAPI CTCPBase::SendProc(void* pParam)
{
	CTCPBase* tcp = (CTCPBase*)pParam;
	while (TRUE)
	{
		if (!tcp->SendProcess(tcp->m_hStopEvent))
		{
			break;
		}
	}

    DBG_INFO(_T("CTCPBase: Send Thread Stop\r\n"));

	tcp->Release();

	return 0;
}

BOOL CTCPBase::SendProcess(HANDLE StopEvent)
{
    BOOL SendOK = TRUE;
	HANDLE h[2] = {
		m_hSendEvent,
		m_hStopEvent,
	};

	DWORD Ret = WaitForMultipleObjects(2, h, FALSE, INFINITE);
	if (Ret != WAIT_OBJECT_0)
	{
		return FALSE;
	}

	EnterCriticalSection(&m_csSendLock);

	if (!m_SendList.empty())
	{
        TCPSendNode node = m_SendList.front();
        BASE_PACKET_T* Packet = node.Packet;
		m_SendList.pop_front();

        m_dwSendBufferSize -= Packet->Length;

		LeaveCriticalSection(&m_csSendLock);

		DWORD Length = Packet->Length;
        PBYTE Data = (BYTE*)Packet;

		while (Length > 0)
		{
			DWORD sendSize = 0;
			BOOL res = TCPSocketWrite(m_hSock, Data, Length, &sendSize, StopEvent);
            if (res)
            {
                Data += sendSize;
                Length -= sendSize;
            }
            else
            {
                SendOK = FALSE;
                break;
            }
		}

        if (node.SyncHandle)
        {
            SetEvent(node.SyncHandle);
        }
		
		free(Packet);
	}
	else
	{
		ResetEvent(m_hSendEvent);
		LeaveCriticalSection(&m_csSendLock);
	}

	return SendOK;
}

BOOL CTCPBase::RecvProcess(HANDLE StopEvent)
{
	BOOL Ret = FALSE;

	BYTE Buffer[4096];
	BASE_PACKET_T* Packet = NULL;

	DWORD recvSize;
	BOOL res = TCPSocketRead(m_hSock, (BYTE*)Buffer, 4096, &recvSize, StopEvent);
	if (!res)
	{
		return FALSE;
	}

	DataStreamBufferAddData(m_hDataStreamBuffer, Buffer, recvSize);
	while (TRUE)
	{
		Packet = GetPacketFromBuffer(m_hDataStreamBuffer);
		if (Packet != NULL)
		{
			EnterCriticalSection(&m_csFunc);
            if (m_pfnRecvFunc)
			{
				Ret = m_pfnRecvFunc(Packet, this, m_pRecvParam);
			}
			LeaveCriticalSection(&m_csFunc);

            free(Packet);
		}
		else
		{
			return TRUE;
		}
	}
	
	return FALSE;
}

VOID CTCPBase::GetSrcPeer(CHAR* SrcAddress, DWORD BufferLen, WORD* SrcPort)
{
    strncpy(SrcAddress, m_szSrcAddress, BufferLen);

    *SrcPort = m_dwSrcPort;

    return;
}

VOID CTCPBase::GetDstPeer(CHAR* DstAddress, DWORD BufferLen, WORD* DstPort)
{
    strncpy(DstAddress, m_szDstAddress, BufferLen);

    *DstPort = m_dwDstPort;

    return;
}