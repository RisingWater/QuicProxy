#pragma once

#ifndef __QUIC_CLIENT_H__
#define __QUIC_CLIENT_H__

#include "QUICBase.h"
#include "QUICChannel.h"
#include "QUICCtrlChannel.h"
#include <map>

class CQUICClient : public IQUICClient, public CQUICBase, public CQUICLink
{
public:
    CQUICClient(CHAR* ipaddr, WORD dstPort, const CHAR* Keyword);

    virtual BOOL Connect();

    virtual VOID Disconnect();

    virtual IQUICChannel* CreateChannel(const CHAR* ChannelName, WORD Priority);

private:
    virtual ~CQUICClient();

    static QUIC_STATUS QUIC_API ClientConnectionCallback(
        _In_ HQUIC Connection,
        _In_opt_ void* Context,
        _Inout_ QUIC_CONNECTION_EVENT* Event
        );
    
    CHAR* m_szIPAddress;

    WORD  m_wDstPort;

    QUIC_BUFFER m_stKeyword;

    VOID LoadConfiguration();

    HQUIC m_hConnection;

    HANDLE m_hConnectOK;
};


#endif