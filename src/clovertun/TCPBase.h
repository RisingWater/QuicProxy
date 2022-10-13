#pragma once

#ifndef __TCP_BASE_H__
#define __TCP_BASE_H__

#ifdef WIN32
#include <WinSock2.h>
#include <windows.h>
#else
#include "winpr/wtypes.h"
#include "winpr/winsock.h"
#include "winpr/thread.h"
#endif

#include "BaseObject.h"
#include "packetunit.h"
#include <list>

class CTCPBase;

typedef BOOL (*_TCPRecvPacketProcess)(BASE_PACKET_T* Packet, CTCPBase* tcp, CBaseObject* Param);
typedef VOID (*_TCPEndProcess)(CTCPBase* tcp, CBaseObject* Param);

typedef struct
{
    BASE_PACKET_T* Packet;
    HANDLE         SyncHandle;
} TCPSendNode;

class CTCPBase : public CBaseObject
{
public :
	CTCPBase();
	virtual ~CTCPBase();

    VOID SendPacket(BASE_PACKET_T* Packet, HANDLE SyncHandle = NULL);
    VOID RegisterRecvProcess(_TCPRecvPacketProcess Process, CBaseObject* Param);
    VOID RegisterEndProcess(_TCPEndProcess Process, CBaseObject* Param);

    VOID GetSrcPeer(CHAR* SrcAddress, DWORD BufferLen, WORD* SrcPort);
    VOID GetDstPeer(CHAR* DstAddress, DWORD BufferLen, WORD* DstPort);

    DWORD GetSendBuffSize();

protected:
    BOOL InitBase();
    VOID DoneBase();

	SOCKET m_hSock;

    CHAR m_szDstAddress[128];
    WORD m_dwDstPort;

    CHAR m_szSrcAddress[128];
    WORD m_dwSrcPort;

private:
    static DWORD WINAPI RecvProc(void* pParam);
    static DWORD WINAPI SendProc(void* pParam);

	BOOL SendProcess(HANDLE StopEvent);
	BOOL RecvProcess(HANDLE StopEvent);

    HANDLE m_hRecvThread;
    HANDLE m_hSendThread;
    HANDLE m_hStopEvent;
    HANDLE m_hSendEvent;

    DWORD m_dwSendBufferSize;

    CRITICAL_SECTION m_csSendLock;
    std::list<TCPSendNode> m_SendList;
    HANDLE m_hDataStreamBuffer;

    CRITICAL_SECTION m_csFunc;
    _TCPRecvPacketProcess m_pfnRecvFunc;
    CBaseObject* m_pRecvParam;
    _TCPEndProcess m_pfnEndFunc;
    CBaseObject* m_pEndParam;
};

#endif

