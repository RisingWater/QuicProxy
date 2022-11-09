#include "stdafx.h"
#include "ProxyProcessor.h"
#include "ClientSession.h"
#include "ProxyService.h"
#include "IOpini.h"

#pragma pack(1)

typedef enum
{
	PROXY_TYPE_CONNECT = 0,
	PROXY_TYPE_DATA = 1,
	PROXY_TYPE_DISCONNECT = 2,
} PROXY_PACKET_TYPE;

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

CProxyProcessor::CProxyProcessor(HANDLE stopEvent, CHAR* address, int port)
{
	m_hStopEvent = stopEvent;
    m_pQuicChannel = new CQUICClient(address, port, "xredtest");

	InitializeCriticalSection(&m_csLock);
    InitializeCriticalSection(&m_csQuicLock);
}

CProxyProcessor::~CProxyProcessor()
{
	DeleteCriticalSection(&m_csLock);
    DeleteCriticalSection(&m_csQuicLock);

    if (m_pQuicChannel)
    {
        m_pQuicChannel->Release();
    }
}

BOOL CProxyProcessor::Init(CHAR* szConfigPath)
{
    EnterCriticalSection(&m_csQuicLock);

    if (m_pQuicChannel)
	{
        m_pQuicChannel->RegisterRecvProcess(CProxyProcessor::QUICClientRecvPacketProcess, this);
        m_pQuicChannel->RegisterEndProcess(CProxyProcessor::QUICDisconnectedProcess, this);
        m_pQuicChannel->Init();
	}

    LeaveCriticalSection(&m_csQuicLock);

    IFileIni* Ini = CreateIFileIniInstance(szConfigPath);

	char service[1024] = { 0 };

#define MAX_SERVICE_COUNT 32

	if (Ini->ReadOption("config", "service", service))
	{
		CHAR szPlugins[MAX_SERVICE_COUNT][MAX_PATH];
		CHAR* tok = NULL;
		DWORD nCount = 0;

		tok = strtok(service, ",");

		while (tok != NULL && nCount < MAX_SERVICE_COUNT)
		{
			strcpy(szPlugins[nCount], tok);
			nCount++;
			tok = strtok(NULL, ",");
		}

		for (DWORD i = 0; i < nCount; i++)
		{
			BOOL ret;
			CHAR ipAddress[128] = { 0 };
			DWORD RemotePort;
			DWORD LocalPort;
			CHAR* szPlugin = szPlugins[i];

			if (strlen(szPlugin) == 0)
			{
				continue;
			}

			ret = Ini->ReadOption(szPlugin, "ip", ipAddress);
			if (!ret)
			{
                DBG_ERROR(_T("read %s[ipAddress] failed\r\n"), szPlugin);
				continue;
			}

			ret = Ini->ReadOption(szPlugin, "remote_port", &RemotePort);
			if (!ret)
			{
                DBG_ERROR(_T("read %s[RemotePort] failed\r\n"), szPlugin);
				continue;
			}

			ret = Ini->ReadOption(szPlugin, "local_port", &LocalPort);
			if (!ret)
			{
                DBG_ERROR(_T("read %s[LocalPort] failed\r\n"), szPlugin);
				continue;
			}

			CProxyService* Service = new CProxyService(ipAddress, RemotePort, LocalPort, this);
			if (Service->Init())
			{
				m_ServiceList.push_back(Service);
			}
			else
			{
				DBG_ERROR(_T("Service Init failed\r\n"));
			}
		}
	}
    
	return TRUE;
}

void CProxyProcessor::Run()
{
	WaitForSingleObject(m_hStopEvent, INFINITE);
}

void CProxyProcessor::Done()
{
	std::list<CProxyService*>::iterator ServiceItor;

	for (ServiceItor = m_ServiceList.begin(); ServiceItor != m_ServiceList.end(); ServiceItor++)
	{
		(*ServiceItor)->Done();
		(*ServiceItor)->Release();
	}

	m_ServiceList.clear();

	std::list<CClientSession*>::iterator Itor;

	EnterCriticalSection(&m_csLock);

	for (Itor = m_SessionList.begin(); Itor != m_SessionList.end(); Itor++)
	{
		(*Itor)->Done();
		(*Itor)->Release();
	}

	m_ServiceList.clear();

	LeaveCriticalSection(&m_csLock);

    EnterCriticalSection(&m_csQuicLock);
    if (m_pQuicChannel)
    {
        m_pQuicChannel->RegisterRecvProcess(NULL, NULL);
        m_pQuicChannel->RegisterEndProcess(NULL, NULL);
        m_pQuicChannel->Done();
    }
    LeaveCriticalSection(&m_csQuicLock);
}

VOID CProxyProcessor::QUICDisconnectedProcess(IQUICCommunication* quic, CBaseObject* Param)
{
    CProxyProcessor* proxy = dynamic_cast<CProxyProcessor*>(Param);

    if (proxy)
    {
        SetEvent(proxy->m_hStopEvent);
    }
}

BOOL CProxyProcessor::QUICClientRecvPacketProcess(PBYTE Data, DWORD Length, IQUICCommunication* quic, CBaseObject* Param)
{
    CProxyProcessor* proxy = dynamic_cast<CProxyProcessor*>(Param);

    if (proxy)
    {
        proxy->SocketRecvCallback(Data, Length);
    }

    return TRUE;
}

CClientSession* CProxyProcessor::FindSessionByGUID(GUID guid)
{
	CClientSession* session = NULL;
	std::list<CClientSession*>::iterator Itor;
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


void CProxyProcessor::AddProxySession(CClientSession* session)
{
	EnterCriticalSection(&m_csLock);
	m_SessionList.push_back(session);
	LeaveCriticalSection(&m_csLock);
}

void CProxyProcessor::RemoveProxySession(CClientSession* session)
{
	std::list<CClientSession*>::iterator Itor;

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

void CProxyProcessor::SocketRecvCallback(PBYTE buffer, int len)
{
	PROXY_HEADER* Header = (PROXY_HEADER*)buffer;

	switch (Header->dwType)
	{
	case PROXY_TYPE_DISCONNECT:
	{
		CClientSession* session = FindSessionByGUID(Header->stGuid);
		if (session != NULL)
		{
            DBG_INFO(GUID_FORMAT _T("Recv Disconnect\r\n"), GUID_STRING(Header->stGuid));
			RemoveProxySession(session);
			session->Done();
			session->Release();
		}
		break;
	}

	case PROXY_TYPE_DATA:
	{
		PROXY_DATA* Data = (PROXY_DATA*)buffer;

		CClientSession* session = FindSessionByGUID(Header->stGuid);
		if (session)
		{
            //DBG_INFO(GUID_FORMAT _T("Recv Data len %d\r\n"), GUID_STRING(Header->stGuid), Data->dwLength);
			session->RecvProcess((char*)Data->Data, Data->dwLength);
			session->Release();
		}
	}
	}
}

void CProxyProcessor::SendDataToProxy(GUID guid, char* buffer, DWORD len)
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

void CProxyProcessor::SendDisconnectToProxy(GUID guid)
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

void CProxyProcessor::SendConnectToProxy(GUID guid, CHAR* ip, DWORD Port)
{
	PROXY_CONNECT Connet;

	memset(&Connet, 0, sizeof(PROXY_CONNECT));

	Connet.Header.dwType = PROXY_TYPE_CONNECT;
	memcpy(&Connet.Header.stGuid, &guid, sizeof(GUID));

	strcpy(Connet.szIPAddress, ip);
	Connet.dwPort = Port;
#ifdef UNICODE
    DBG_INFO(GUID_FORMAT _T("Send connect to %S:%d\r\n"), GUID_STRING(guid), ip, Port);
#else
    DBG_INFO(GUID_FORMAT _T("Send connect to %s:%d\r\n"), GUID_STRING(guid), ip, Port);
#endif

    EnterCriticalSection(&m_csQuicLock);

    if (m_pQuicChannel)
    {
        m_pQuicChannel->SendPacket((PBYTE)&Connet, sizeof(PROXY_CONNECT), NULL);
    }

    LeaveCriticalSection(&m_csQuicLock);
}