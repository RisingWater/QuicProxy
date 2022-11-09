#ifndef WIN32
#include <winpr/wtypes.h>
#else
#include <windows.h>
#include <tchar.h>
#endif

#include "BaseObject.h"
#include "IThread.h"

class CProxyProcessor;

class CClientSession : public CBaseObject
{
public:
    CClientSession(SOCKET s, CHAR* m_szIPAddress, DWORD Port, CProxyProcessor* Processor);

    ~CClientSession();

	BOOL Init();

	void Done();

	void GetGuid(GUID* guid);

	void RecvProcess(char* buffer, DWORD len);

private:
	static BOOL SocketRecvProcessDelegate(LPVOID Parameter, HANDLE StopEvent);

	BOOL SocketRecvProcess();

	GUID  m_stGuid;

	CHAR m_szIPAddress[32];

	DWORD m_dwPort;

	SOCKET m_hSocket;

	IThread* m_pRecvThread;

	CProxyProcessor* m_pProcessor;
};