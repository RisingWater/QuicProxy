#ifndef WIN32
#include <winpr/wtypes.h>
#else
#include <windows.h>
#include <tchar.h>
#endif

#include <list>
#include "BaseObject.h"
#include "IThread.h"

class CProxyProcessor;

class CProxyService : public CBaseObject
{
public:
	CProxyService(CHAR* targetip, DWORD remotePort, DWORD localPort, CProxyProcessor* Processor);

	virtual ~CProxyService();

	BOOL Init();

	void Done();

private:
	static BOOL ServiceMainThreadProc(LPVOID Parameter, HANDLE StopEvent);

	IThread* m_pServiceThread;

	CRITICAL_SECTION m_csLock;

	CHAR m_szTargetIP[32];

	DWORD m_dwRemotePort;

	DWORD m_dwLocalPort;

	SOCKET m_hListenSocket;

	CProxyProcessor* m_pProcessor;
};