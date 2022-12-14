#pragma once

#ifndef __ENET_CLIENT_H__
#define __ENET_CLIENT_H__

#ifdef WIN32
#include <WinSock2.h>
#include <windows.h>
#else
#include <winpr/wtypes.h>
#endif

#include <enet/enet.h>
#include "BasePacket.h"
#include "ClovertunProtocol.h"
#include "BaseObject.h"
#include <list>

typedef struct
{
    ENetPacket* Packet;
    HANDLE      SyncHandle;
} ENetSendNode;

class CENetClient;
typedef BOOL (*_ENetRecvPacketProcess)(PBYTE Data, DWORD Length, CENetClient* tcp, CBaseObject* Param);

class CENetClient : public CBaseObject
{
public :
    CENetClient(DWORD PeerId, CLIENT_INFO info, BOOL IsHost);
	virtual ~CENetClient();

    BOOL Init();
    VOID Done();

    VOID SendPacket(PBYTE Data, DWORD Length, HANDLE SyncHandle = NULL);
    VOID RegisterRecvProcess(_ENetRecvPacketProcess Process, CBaseObject* Param);

private:

    static DWORD WINAPI MainProc(void* pParam);
    
	BOOL ENetProcess(HANDLE StopEvent);
    
    HANDLE m_hMainThread;
    HANDLE m_hStopEvent;
    
    CRITICAL_SECTION m_csSendLock;
    std::list<ENetSendNode> m_SendList;

    CRITICAL_SECTION m_csLock;
    _ENetRecvPacketProcess m_pfnRecvFunc;
    CBaseObject* m_pRecvParam;

    DWORD m_dwPeerId;
    CLIENT_INFO m_stPeerInfo;
    BOOL m_bIsHost;
    ENetHost* m_pENetHost;
    ENetPeer* m_pENetPeer;
};

#endif

