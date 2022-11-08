#ifndef __SOCKET_PROXY_H__
#define __SOCKET_PROXY_H__

#include "BaseObject.h"
#include "IThread.h"
#include <list>

class CSocketProxy;
class CQUICServer;
class IQUICCommunication;

class CProxySession : public CBaseObject
{
public:
	CProxySession(char* ipaddr, DWORD port, GUID guid, CSocketProxy* Proxy);

	virtual ~CProxySession();

	void GetGuid(GUID* guid);

	BOOL Init();

	void Done();

	void RecvProcess(char* buffer, DWORD len);

private:
	static BOOL SocketRecvProcessDelegate(LPVOID Parameter, HANDLE StopEvent);

	BOOL SocketRecvProcess();

	CHAR   m_szIPAddress[32];
	DWORD  m_dwPort;
	GUID   m_stGuid;
	SOCKET m_hSocket;
	IThread* m_pRecvThread;

	CSocketProxy* m_pProxy;
};

class CSocketProxy : public CBaseObject
{
public:
    CSocketProxy(CQUICServer* quicChannel);

	virtual ~CSocketProxy();

	BOOL Init();

	void Done();

	void SendDataToProxy(GUID guid, char* buffer, DWORD len);

	void SendDisconnectToProxy(GUID guid);

private:
    static BOOL QUICServerRecvPacketProcess(PBYTE Data, DWORD Length, IQUICCommunication* quic, CBaseObject* Param);

    static VOID QUICDisconnectedProcess(IQUICCommunication* quic, CBaseObject* Param);

    void SocketRecvCallback(PBYTE buffer, DWORD len);

	CProxySession* FindSessionByGUID(GUID guid);

	void AddProxySession(CProxySession* session);

	void RemoveProxySession(CProxySession* session);

	std::list<CProxySession*> m_SessionList;

    CQUICServer* m_pQuicChannel;

	CRITICAL_SECTION m_csLock;
};

#endif