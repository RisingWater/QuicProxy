#include "stdafx.h"
#include "ClientSession.h"
#include "ProxyService.h"
#include "ProxyProcessor.h"

CProxyService::CProxyService(CHAR* targetip, DWORD RemotePort, DWORD localPort, CProxyProcessor* Processor)
{
	strcpy(m_szTargetIP, targetip);
	m_dwRemotePort = RemotePort;
	m_dwLocalPort = localPort;

	m_pProcessor = Processor;
	if (m_pProcessor)
	{
		m_pProcessor->AddRef();
	}

	m_pServiceThread = CreateIThreadInstance(CProxyService::ServiceMainThreadProc, this);
}

CProxyService::~CProxyService()
{
	if (m_pProcessor)
	{
		m_pProcessor->Release();
	}
}

BOOL CProxyService::ServiceMainThreadProc(LPVOID Parameter, HANDLE StopEvent)
{
	CProxyService* Service = (CProxyService*)Parameter;

	while (TRUE)
	{
		DWORD Ret = WaitForSingleObject(StopEvent, 0);
		if (Ret != WAIT_TIMEOUT)
		{
			break;
		}

		SOCKET s = accept(Service->m_hListenSocket, NULL, NULL);

		if (s == NULL || s == INVALID_SOCKET)
		{
			break;
		}

		//when socket server disconnect, it CClientSession release itself
		CClientSession* Session = new CClientSession(s, Service->m_szTargetIP, Service->m_dwRemotePort, Service->m_pProcessor);
		if (Session->Init())
		{
			DBG_INFO(_T("new connection\r\n"));
			Service->m_pProcessor->AddProxySession(Session);
		}
		else
		{
			Session->Release();
		}
	}

	Service->Release();
	return FALSE;
}

BOOL CProxyService::Init()
{
	struct sockaddr_in sockAddr;
	SOCKET sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);

	if (sock == INVALID_SOCKET)
	{
		return FALSE;
	}

	int flag = 1;
	setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, (const char*)&flag, sizeof(int));

	sockAddr.sin_addr.s_addr = INADDR_ANY;
	sockAddr.sin_port = htons((WORD)m_dwLocalPort);
	sockAddr.sin_family = AF_INET;
	int Ret = bind(sock, (struct sockaddr *)&sockAddr, sizeof(sockAddr));

	if (Ret < 0)
	{
		closesocket(sock);
		return FALSE;
	}

	listen(sock, 0x10);

	m_hListenSocket = sock;

	DBG_INFO(_T("create socket listen %d ok\r\n"), m_dwLocalPort);

	AddRef();
	m_pServiceThread->StartMainThread();
    
	return TRUE;
}

void CProxyService::Done()
{
	closesocket(m_hListenSocket);
}