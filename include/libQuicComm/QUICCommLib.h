#ifndef __QUIC_COMM_LIB_H__
#define __QUIC_COMM_LIB_H__

#ifdef WIN32
#define DLL_QUIC_COMM_LIB_API __declspec(dllexport)
#else
#define DLL_QUIC_COMM_LIB_API __attribute__ ((visibility("default")))
#endif

#include "BaseObject.h"

class IQUICChannel;

class IQUICLink;

typedef BOOL(*_QUICRecvPacketProcess)(PBYTE Data, DWORD Length, IQUICChannel* quic, CBaseObject* Param);

typedef void(*_QUICDisconnectedProcess)(IQUICChannel* s, CBaseObject* pParam);

typedef void(*_QUICLinkDisconnectedProcess)(IQUICLink* s, CBaseObject* pParam);

class IQUICChannel : public virtual CBaseObject
{
public:
    virtual const CHAR* GetChannelName() = 0;

    virtual WORD GetPriority() = 0;

    virtual BOOL SendPacket(PBYTE Data, DWORD Length, HANDLE SyncHandle = NULL) = 0;

    virtual BOOL SendPacketSync(PBYTE Data, DWORD Length) = 0;

    virtual VOID RegisterRecvProcess(_QUICRecvPacketProcess Process, CBaseObject* Param) = 0;

    virtual VOID RegisterEndProcess(_QUICDisconnectedProcess Process, CBaseObject* Param) = 0;
};

class IQUICLink : public virtual CBaseObject
{
public:
    virtual BOOL SendCtrlPacket(PBYTE Data, DWORD DataLen) = 0;

    virtual PBYTE RecvCtrlPacket(DWORD* DataLen, DWORD TimeOut) = 0;

    virtual void FreeRecvedCtrlPacket(PBYTE Data) = 0;

    virtual void DestoryChannel(IQUICChannel* channel) = 0;

    virtual VOID RegisterEndProcess(_QUICLinkDisconnectedProcess Process, CBaseObject* Param) = 0;
};

class IQUICClient : public virtual IQUICLink
{
public:
    virtual BOOL Connect() = 0;

    virtual VOID Disconnect() = 0;

    virtual IQUICChannel* CreateChannel(const CHAR* ChannelName, WORD Priority) = 0;
};

class IQUICServer : public virtual IQUICLink
{
public:
    virtual VOID Disconnect() = 0;

    virtual IQUICChannel* WaitForChannelReady(const CHAR* channelName, HANDLE hStopEvent, DWORD TimeOut = 0xFFFFFFFF) = 0;

    virtual BOOL GetRemotePeerInfo(CHAR* Address, DWORD* Port) = 0;
};

class IQUICService : public virtual CBaseObject
{
public:
    virtual BOOL StartListen() = 0;

    virtual VOID StopListen() = 0;

    virtual IQUICServer* Accept(HANDLE StopEvent) = 0;
};

extern "C"
{
    DLL_QUIC_COMM_LIB_API  IQUICService* WINAPI CreateIQUICService(WORD Port, const CHAR* Keyword, const CHAR* CertFilePath, const CHAR* KeyFilePath);
    DLL_QUIC_COMM_LIB_API  IQUICClient* WINAPI CreateIQUICClient(CHAR* ipaddr, WORD dstPort, const CHAR* Keyword);
}

#endif