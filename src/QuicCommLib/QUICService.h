#pragma once

#ifndef __QUIC_SERVICE_H__
#define __QUIC_SERVICE_H__

#include "QUICBase.h"
#include "QUICServer.h"
#include "QUICCommLib.h"
#include <map>

class CQUICService : public CQUICBase, public IQUICService
{
public:
    CQUICService(WORD Port, const CHAR* Keyword, const CHAR* CertFilePath, const CHAR* KeyFilePath);

    virtual ~CQUICService();

    virtual BOOL StartListen();

    virtual VOID StopListen();

    virtual IQUICServer* Accept(HANDLE StopEvent);

private:
    VOID LoadConfiguration();

    CQUICServer* CreateQUICServer(HQUIC Connection);

    static QUIC_STATUS QUIC_API ServerListenerCallback(
        _In_ HQUIC Listener,
        _In_opt_ void* Context,
        _Inout_ QUIC_LISTENER_EVENT* Event
        );

    WORD m_wUdpPort;

    HQUIC m_hListener;

    CHAR* m_szCertFilePath;

    CHAR* m_szKeyFilePath;

    QUIC_BUFFER m_stKeyword;

    CRITICAL_SECTION m_csLock;

    std::list<CQUICServer*> m_ServerList;

    HANDLE m_hAcceptEvent;

    HANDLE m_hStopEvent;
};


#endif