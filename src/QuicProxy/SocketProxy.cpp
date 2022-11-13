#include "stdafx.h"
#include "SocketProxy.h"
#include "QUICCommLib.h"

#pragma pack(1)

typedef enum
{
	PROXY_TYPE_CONNECT = 0,
	PROXY_TYPE_DATA = 1,
	PROXY_TYPE_DISCONNECT = 2,
};

typedef struct
{
	DWORD dwType;
	GUID  stGuid;
} PROXY_HEADER;

typedef struct
{
	PROXY_HEADER Header;
	CHAR  szIPAddress[32];
	DWORD dwPort;
} PROXY_CONNECT, PROXY_DISCONNECT;

typedef struct
{
	PROXY_HEADER Header;
	DWORD dwLength;
	BYTE  Data[0];
} PROXY_DATA;

#pragma pack()

CProxySession::CProxySession(char* ipaddr, DWORD port, GUID guid, CSocketProxy* Proxy)
{
	strcpy(m_szIPAddress, ipaddr);
	m_dwPort = port;
	memcpy(&m_stGuid, &guid, sizeof(GUID));
	m_hSocket = NULL;

	m_pRecvThread = CreateIThreadInstance(CProxySession::SocketRecvProcessDelegate, this);

	m_pProxy = Proxy;
	if (m_pProxy)
	{
		m_pProxy->AddRef();
	}
}

CProxySession::~CProxySession()
{
	if (m_hSocket)
	{
		shutdown(m_hSocket, SD_BOTH);
		closesocket(m_hSocket);
		m_hSocket = NULL;
	}

	if (m_pRecvThread)
	{
		m_pRecvThread->Release();
		m_pRecvThread = NULL;
	}

	if (m_pProxy)
	{
		m_pProxy->Release();
		m_pProxy = NULL;
	}
}

void CProxySession::GetGuid(GUID* guid)
{
	memcpy(guid, &m_stGuid, sizeof(GUID));
}

BOOL CProxySession::Init()
{
	struct sockaddr_in  DstAddress;

	int Ret;
	SOCKET sock = socket(AF_INET, SOCK_STREAM, 0);

	if (sock < 0)
	{
		return FALSE;
	}

	memset(&DstAddress, 0, sizeof(sockaddr_in));
	DstAddress.sin_family = AF_INET;
	DstAddress.sin_port = htons((WORD)m_dwPort);
	DstAddress.sin_addr.s_addr = inet_addr(m_szIPAddress);

	Ret = connect(sock, (struct sockaddr *)&DstAddress, sizeof(sockaddr_in));
	if (Ret < 0)
	{
#ifdef UNICODE
        DBG_TRACE(_T("connect to %S:%d failed\r\n"), m_szIPAddress, m_dwPort);
#else
		DBG_TRACE(_T("connect to %s:%d failed\r\n"), m_szIPAddress, m_dwPort);
#endif
		closesocket(sock);
		return FALSE;
	}

	m_hSocket = sock;

#ifdef UNICODE
    DBG_TRACE(_T("connect to %S:%d ok\r\n"), m_szIPAddress, m_dwPort);
#else
	DBG_TRACE(_T("connect to %s:%d ok\r\n"), m_szIPAddress, m_dwPort);
#endif

	if (m_pRecvThread)
	{
		AddRef();
		m_pRecvThread->StartMainThread();
	}

	return TRUE;
}

void CProxySession::Done()
{
	if (m_hSocket)
	{
		shutdown(m_hSocket, SD_BOTH);
		closesocket(m_hSocket);
		m_hSocket = NULL;
	}

	if (m_pRecvThread)
	{
		m_pRecvThread->StopMainThread();
	}
}

BOOL CProxySession::SocketRecvProcessDelegate(LPVOID Parameter, HANDLE StopEvent)
{
	CProxySession* session = (CProxySession*)Parameter;

	if (session)
	{
		if (!session->SocketRecvProcess())
		{
			session->m_pProxy->SendDisconnectToProxy(session->m_stGuid);
			session->Done();
			session->Release();
			return FALSE;
		}
		else
		{
			return TRUE;
		}
	}
	else
	{
		return FALSE;
	}
}

BOOL CProxySession::SocketRecvProcess()
{
	char data[4096];
	int recvlen = recv(m_hSocket, data, 4096, 0);

	if (recvlen <= 0)
	{
		DBG_TRACE(_T("session recv failed\r\n"));
		return FALSE;
	}

	m_pProxy->SendDataToProxy(m_stGuid, data, recvlen);
    
	return TRUE;
}

void CProxySession::RecvProcess(char* buffer, DWORD len)
{
	int left = len;
	char* ptr = buffer;

	while (left > 0)
	{
    	int sendlen = send(m_hSocket, ptr, left, 0);
    	if (sendlen <= 0)
		{
			DBG_TRACE(_T("session send failed\r\n"));
			return;
		}

		ptr += sendlen;
		left -= sendlen;
	}

	return;
}

CSocketProxy::CSocketProxy(IQUICServer* quicChannel)
{
    InitializeCriticalSection(&m_csLock);
    InitializeCriticalSection(&m_csQuicLock);

    EnterCriticalSection(&m_csQuicLock);
    m_pQuicServer = quicChannel;
    LeaveCriticalSection(&m_csQuicLock);

    m_hStopEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
}

CSocketProxy::~CSocketProxy()
{
    if (m_pQuicServer)
    {
        m_pQuicServer->Release();
    }

    if (m_hStopEvent)
    {
        CloseHandle(m_hStopEvent);
    }

	DeleteCriticalSection(&m_csLock);
    DeleteCriticalSection(&m_csQuicLock);
}

BOOL CSocketProxy::Init()
{
    EnterCriticalSection(&m_csQuicLock);

    if (m_pQuicServer)
    {
        m_pQuicChannel = m_pQuicServer->WaitForChannelReady("control", m_hStopEvent, 5000);
        if (m_pQuicChannel)
        {
            m_pQuicChannel->RegisterRecvProcess(CSocketProxy::QUICServerRecvPacketProcess, this);
            m_pQuicChannel->RegisterEndProcess(CSocketProxy::QUICDisconnectedProcess, this);
        }
        m_pQuicServer->SendCtrlPacket((PBYTE)"ctrl channel init ok", strlen("ctrl channel init ok") + 1);
    }

    LeaveCriticalSection(&m_csQuicLock);

	return TRUE;
}

void CSocketProxy::Done()
{
    std::list<CProxySession*>::iterator Itor;
    CProxySession* session = NULL;
    
    EnterCriticalSection(&m_csLock);

    for (Itor = m_SessionList.begin(); Itor != m_SessionList.end(); Itor++)
    {
        session = (*Itor);
        session->Done();
        session->Release();
    }

    m_SessionList.clear();

    LeaveCriticalSection(&m_csLock);

    EnterCriticalSection(&m_csQuicLock);

    if (m_pQuicServer)
    {
        if (m_pQuicChannel)
        {
            m_pQuicChannel->RegisterRecvProcess(NULL, NULL);
            m_pQuicChannel->RegisterEndProcess(NULL, NULL);
            m_pQuicServer->DestoryChannel(m_pQuicChannel);
            m_pQuicChannel->Release();
            m_pQuicChannel = NULL;
        }

        m_pQuicServer->Disconnect();
    }

    LeaveCriticalSection(&m_csQuicLock);

    Release();
}

CProxySession* CSocketProxy::FindSessionByGUID(GUID guid)
{
	CProxySession* session = NULL;
	std::list<CProxySession*>::iterator Itor;
	EnterCriticalSection(&m_csLock);

	for (Itor = m_SessionList.begin(); Itor != m_SessionList.end(); Itor++)
	{
		GUID session_guid;
		(*Itor)->GetGuid(&session_guid);
		if (memcmp(&guid, &session_guid, sizeof(GUID)) == 0)
		{
			(*Itor)->AddRef();
			session = (*Itor);
		}
	}

	LeaveCriticalSection(&m_csLock);

	return session;
}

void CSocketProxy::AddProxySession(CProxySession* session)
{
	EnterCriticalSection(&m_csLock);
	m_SessionList.push_back(session);
	LeaveCriticalSection(&m_csLock);
}

void CSocketProxy::RemoveProxySession(CProxySession* session)
{
	std::list<CProxySession*>::iterator Itor;

	GUID guid;
	session->GetGuid(&guid);

	EnterCriticalSection(&m_csLock);

	for (Itor = m_SessionList.begin(); Itor != m_SessionList.end(); Itor++)
	{
		GUID session_guid;
		(*Itor)->GetGuid(&session_guid);
		if (memcmp(&guid, &session_guid, sizeof(GUID)) == 0)
		{
			m_SessionList.erase(Itor);
			break;
		}
	}

	LeaveCriticalSection(&m_csLock);

	return;
}

BOOL CSocketProxy::QUICServerRecvPacketProcess(PBYTE Data, DWORD Length, IQUICChannel* quic, CBaseObject* Param)
{
    CSocketProxy* proxy = dynamic_cast<CSocketProxy*>(Param);

	if (proxy)
	{
        proxy->SocketRecvCallback(Data, Length);
	}

    return TRUE;
}

VOID CSocketProxy::QUICDisconnectedProcess(IQUICChannel* quic, CBaseObject* Param)
{
    CSocketProxy* proxy = dynamic_cast<CSocketProxy*>(Param);

    if (proxy)
    {
        proxy->Done();
    }
}

void CSocketProxy::SocketRecvCallback(PBYTE buffer, DWORD len)
{
	PROXY_HEADER* Header = (PROXY_HEADER*)buffer;
    
	switch (Header->dwType)
	{
		case PROXY_TYPE_CONNECT:
		{
            DBG_INFO(GUID_FORMAT _T("Recv Connect\r\n"), GUID_STRING(Header->stGuid));
			CProxySession* session = FindSessionByGUID(Header->stGuid);
			if (session != NULL)
			{
				RemoveProxySession(session);
				session->Done();
				session->Release();
			}

			PROXY_CONNECT* Connect = (PROXY_CONNECT*)buffer;

			session = new CProxySession(Connect->szIPAddress, Connect->dwPort, Connect->Header.stGuid, this);
			if (!session->Init())
			{
				DBG_TRACE(_T("new session init failed\r\n"));
				session->Done();
				session->Release();
			}
			else
			{
				DBG_TRACE(_T("new session init ok\r\n"));
				AddProxySession(session);
			}
			break;
		}

		case PROXY_TYPE_DISCONNECT:
		{
            DBG_INFO(GUID_FORMAT _T("Recv Disconnect\r\n"), GUID_STRING(Header->stGuid));
			CProxySession* session = FindSessionByGUID(Header->stGuid);
			if (session != NULL)
			{
				RemoveProxySession(session);
				session->Done();
				session->Release();
			}
			break;
		}

		case PROXY_TYPE_DATA:
		{
			PROXY_DATA* Data = (PROXY_DATA*)buffer;

			CProxySession* session = FindSessionByGUID(Header->stGuid);
			if (session)
			{
                //DBG_INFO(GUID_FORMAT _T("Recv Data len %d\r\n"), GUID_STRING(Header->stGuid), Data->dwLength);
				session->RecvProcess((char*)Data->Data, Data->dwLength);
				session->Release();
			}
		}
	}
}

void CSocketProxy::SendDataToProxy(GUID guid, char* buffer, DWORD len)
{
	DWORD pktLen = sizeof(PROXY_DATA) + len;

    PBYTE pkt = (PBYTE)malloc(pktLen);

	PROXY_DATA* header = (PROXY_DATA*)pkt;
	header->Header.dwType = PROXY_TYPE_DATA;
	memcpy(&header->Header.stGuid, &guid, sizeof(GUID));
	header->dwLength = len;
	memcpy(header->Data, buffer, len);

    //DBG_INFO(GUID_FORMAT _T("Send Data len %d\r\n"), GUID_STRING(guid), len);

    EnterCriticalSection(&m_csQuicLock);

    if (m_pQuicChannel)
    {
        m_pQuicChannel->SendPacket(pkt, pktLen, NULL);
    }

    LeaveCriticalSection(&m_csQuicLock);

	free(pkt);
}

void CSocketProxy::SendDisconnectToProxy(GUID guid)
{
	PROXY_DISCONNECT Disconnet;

	memset(&Disconnet, 0, sizeof(PROXY_DISCONNECT));

	Disconnet.Header.dwType = PROXY_TYPE_DISCONNECT;
	memcpy(&Disconnet.Header.stGuid, &guid, sizeof(GUID));

    DBG_INFO(GUID_FORMAT _T("Send Disconnect\r\n"), GUID_STRING(guid));

    EnterCriticalSection(&m_csQuicLock);

    if (m_pQuicChannel)
    {
        m_pQuicChannel->SendPacket((PBYTE)&Disconnet, sizeof(PROXY_DISCONNECT), NULL);
    }

    LeaveCriticalSection(&m_csQuicLock);
}
