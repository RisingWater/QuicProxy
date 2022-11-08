#include "stdafx.h"
#include "SocketProxy.h"
#include "QUICServer.h"

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
	struct sockaddr_in  SrcAddress;

	int Ret;
	SOCKET sock = socket(AF_INET, SOCK_STREAM, 0);

	if (sock < 0)
	{
		return FALSE;
	}

	memset(&DstAddress, 0, sizeof(sockaddr_in));
	DstAddress.sin_family = AF_INET;
	DstAddress.sin_port = htons(m_dwPort);
	DstAddress.sin_addr.s_addr = inet_addr(m_szIPAddress);

	Ret = connect(sock, (struct sockaddr *)&DstAddress, sizeof(sockaddr_in));
	if (Ret < 0)
	{
		DBG_TRACE(_T("connect to %s:%d failed\r\n"), m_szIPAddress, m_dwPort);
		closesocket(sock);
		return FALSE;
	}

	m_hSocket = sock;

	DBG_TRACE(_T("connect to %s:%d ok\r\n"), m_szIPAddress, m_dwPort);

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

CSocketProxy::CSocketProxy(CQUICServer* quicChannel)
{
    m_pQuicChannel = quicChannel;
	InitializeCriticalSection(&m_csLock);
}

CSocketProxy::~CSocketProxy()
{
	DeleteCriticalSection(&m_csLock);
}

BOOL CSocketProxy::Init()
{
    m_pQuicChannel->RegisterRecvProcess(CSocketProxy::QUICServerRecvPacketProcess, this);
    m_pQuicChannel->RegisterEndProcess(CSocketProxy::QUICDisconnectedProcess, this);
	return TRUE;
}

void CSocketProxy::Done()
{
    m_pQuicChannel->RegisterRecvProcess(NULL, NULL);
    m_pQuicChannel->RegisterEndProcess(NULL, NULL);
    m_pQuicChannel->CloseConnection();
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

BOOL CSocketProxy::QUICServerRecvPacketProcess(PBYTE Data, DWORD Length, IQUICCommunication* quic, CBaseObject* Param)
{
    CSocketProxy* proxy = dynamic_cast<CSocketProxy*>(Param);

	if (proxy)
	{
        proxy->SocketRecvCallback(Data, Length);
	}
}

void CSocketProxy::SocketRecvCallback(PBYTE buffer, DWORD len)
{
	PROXY_HEADER* Header = (PROXY_HEADER*)buffer;

	switch (Header->dwType)
	{
		case PROXY_TYPE_CONNECT:
		{
			DBG_TRACE(_T("new session connect\r\n"));
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
			DBG_TRACE(_T("new session disconnect\r\n"));
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

	m_pQuicChannel->SendPacket(pkt, pktLen, NULL);

	free(pkt);
}

void CSocketProxy::SendDisconnectToProxy(GUID guid)
{
	PROXY_DISCONNECT Disconnet;

	memset(&Disconnet, 0, sizeof(PROXY_DISCONNECT));

	Disconnet.Header.dwType = PROXY_TYPE_DISCONNECT;
	memcpy(&Disconnet.Header.stGuid, &guid, sizeof(GUID));

    m_pQuicChannel->SendPacket((PBYTE)&Disconnet, sizeof(PROXY_DISCONNECT), NULL);
}
