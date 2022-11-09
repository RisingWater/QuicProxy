#ifndef WIN32
#include <winpr/wtypes.h>
#else
#include <windows.h>
#include <tchar.h>
#endif

#include <list>

#include "BaseObject.h"
#include "QUICClient.h"

class CProxyService;
class CClientSession;

class CProxyProcessor : public CBaseObject
{
public:
	CProxyProcessor(HANDLE stopEvent, CHAR* address, int port);

	virtual ~CProxyProcessor();

	BOOL Init(CHAR* ConfigPath);

	void Run();

	void Done();

    void AddProxySession(CClientSession* session);

    void RemoveProxySession(CClientSession* session);

	void SendDataToProxy(GUID guid, char* buffer, DWORD len);

	void SendDisconnectToProxy(GUID guid);

	void SendConnectToProxy(GUID guid, CHAR* ip, DWORD Port);

private:
    static BOOL QUICClientRecvPacketProcess(PBYTE Data, DWORD Length, IQUICCommunication* quic, CBaseObject* Param);

    static VOID QUICDisconnectedProcess(IQUICCommunication* quic, CBaseObject* Param);
    
    CClientSession* FindSessionByGUID(GUID guid);

	void SocketRecvCallback(PBYTE buffer, int len);

    std::list<CClientSession*> m_SessionList;

	std::list<CProxyService*> m_ServiceList;

	HANDLE m_hStopEvent;

    CQUICClient* m_pQuicChannel;

	CRITICAL_SECTION m_csLock;

    CRITICAL_SECTION m_csQuicLock;
};