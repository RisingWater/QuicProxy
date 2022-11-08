#pragma once

#ifndef __QUIC_SERVER_H__
#define __QUIC_SERVER_H__

#include "QUICBase.h"
#include <map>

class CQUICService;

class CQUICServer : public IQUICCommunication, public CBaseObject
{
public:
    CQUICServer(HQUIC Stream, CQUICService* pService);
    virtual ~CQUICServer();

    void CloseConnection();

    virtual BOOL SendPacket(PBYTE Data, DWORD Length, HANDLE SyncHandle = NULL);

private:
    static QUIC_STATUS QUIC_API ServerStreamCallback(
        _In_ HQUIC Stream,
        _In_opt_ void* Context,
        _Inout_ QUIC_STREAM_EVENT* Event
        );

    HQUIC m_hStream;
    CQUICService* m_pService;

    friend class CQUICService;
};

typedef void (*_ClientConnectedProcess)(CQUICServer* s, CBaseObject* pParam);

class CQUICService : public IQUICBase
{
public:
    CQUICService(WORD Port, const CHAR* Keyword, CHAR* CertFilePath, CHAR* KeyFilePath);
    virtual ~CQUICService();

    virtual BOOL Init();
    virtual VOID Done();

    VOID RegisterConnectedProcess(_ClientConnectedProcess Process, CBaseObject* pParam);

private:
    VOID LoadConfiguration();

    void ServerDisconnect(HQUIC Stream);

    static QUIC_STATUS QUIC_API ServerListenerCallback(
        _In_ HQUIC Listener,
        _In_opt_ void* Context,
        _Inout_ QUIC_LISTENER_EVENT* Event
        );

    static QUIC_STATUS QUIC_API ServerConnectionCallback(
        _In_ HQUIC Connection,
        _In_opt_ void* Context,
        _Inout_ QUIC_CONNECTION_EVENT* Event
        );


    CQUICServer* CreateQUICServer(HQUIC Connection);
    
    WORD m_wUdpPort;
    HQUIC m_hListener;
    CHAR* m_szCertFilePath;
    CHAR* m_szKeyFilePath;

    QUIC_BUFFER m_stKeyword;

    CRITICAL_SECTION m_csLock;
    _ClientConnectedProcess m_pfnAcceptFunc;
    CBaseObject* m_pAcceptParam;

    std::map<HQUIC, CQUICServer*> m_ServerMap;

    friend class CQUICServer;

};


#endif