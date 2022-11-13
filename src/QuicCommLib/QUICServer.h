#pragma once

#ifndef __QUIC_SERVER_H__
#define __QUIC_SERVER_H__

#include "QUICBase.h"
#include "QUICChannel.h"
#include "QUICCtrlChannel.h"
#include <map>

typedef struct
{
    CHAR szChannel[256];
    HANDLE WaitEvent;
} ChannelWaitNode;

class CQUICService;

class CQUICServer : public IQUICServer, public CQUICLink
{
public:
    virtual void Disconnect();

    virtual IQUICChannel* WaitForChannelReady(const CHAR* channelName, HANDLE hStopEvent, DWORD TimeOut = 0xFFFFFFFF);
    
private:
    CQUICServer(HQUIC hConnection, const QUIC_API_TABLE* pMsQuic, CQUICService* pService);

    virtual ~CQUICServer();

    BOOL Init(HQUIC hConfiguration);

    BOOL WaitForServerReady(HANDLE hStopEvent);

    static QUIC_STATUS QUIC_API ServerConnectionCallback(
        _In_ HQUIC Connection,
        _In_opt_ void* Context,
        _Inout_ QUIC_CONNECTION_EVENT* Event
        );

    void CreateChannel(HQUIC Stream);

    IQUICChannel* FindChannelByName(const CHAR* ChannelName);

    const QUIC_API_TABLE*         m_pMsQuic;
    
    HQUIC                         m_hConnection;

    CQUICService*                 m_pService;

    HANDLE                        m_hCtrlChannelReady;

    friend class CQUICService;
};

#endif