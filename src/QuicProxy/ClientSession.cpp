#include "stdafx.h"
#include "ProxyProcessor.h"
#include "ClientSession.h"

CClientSession::CClientSession(SOCKET s, CHAR* IPAddress, DWORD Port, CProxyProcessor* Processor)
{
	m_hSocket = s;
	strcpy(m_szIPAddress, IPAddress);
	m_dwPort = Port;
	m_pProcessor = Processor;
	if (m_pProcessor)
	{
		m_pProcessor->AddRef();
	}

	CoCreateGuid(&m_stGuid);

	m_pRecvThread = CreateIThreadInstance(CClientSession::SocketRecvProcessDelegate, this);
}

CClientSession::~CClientSession()
{
	if (m_pProcessor)
	{
		m_pProcessor->Release();
	}

	if (m_pRecvThread)
	{
		m_pRecvThread->Release();
	}

	if (m_hSocket)
	{
		shutdown(m_hSocket, SD_BOTH);
		closesocket(m_hSocket);
		m_hSocket = NULL;
	}
}

BOOL CClientSession::Init()
{
	m_pProcessor->SendConnectToProxy(m_stGuid, m_szIPAddress, m_dwPort);

	AddRef();
	m_pRecvThread->StartMainThread();

	return TRUE;
}

void CClientSession::Done()
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
		m_pRecvThread = NULL;
	}
}

void CClientSession::GetGuid(GUID* guid)
{
	memcpy(guid, &m_stGuid, sizeof(GUID));
}

void CClientSession::RecvProcess(char* buffer, DWORD len)
{
	int left = len;
	char* ptr = buffer;
	while (left > 0)
	{
		int sendlen = send(m_hSocket, ptr, left, 0);
		if (sendlen <= 0)
		{
			return;
		}

		ptr += sendlen;
		left -= sendlen;
	}

	return;
}


BOOL CClientSession::SocketRecvProcessDelegate(LPVOID Parameter, HANDLE StopEvent)
{
	CClientSession* session = (CClientSession*)Parameter;

	if (session)
	{
		if (!session->SocketRecvProcess())
		{
			session->m_pProcessor->SendDisconnectToProxy(session->m_stGuid);
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

BOOL CClientSession::SocketRecvProcess()
{
	char data[4096];
	int recvlen = recv(m_hSocket, data, 4096, 0);

	if (recvlen <= 0)
	{
		return FALSE;
	}

	m_pProcessor->SendDataToProxy(m_stGuid, data, recvlen);

	return TRUE;
}