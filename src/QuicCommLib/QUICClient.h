#pragma once

#ifndef __QUIC_CLIENT_H__
#define __QUIC_CLIENT_H__

#include "QUICBase.h"

class CQUICClient : public IQUICBase, public IQUICCommunication
{
public:
    CQUICClient(CHAR* ipaddr, WORD dstPort, const CHAR* Keyword);
    virtual ~CQUICClient();

    virtual BOOL Init();
    virtual VOID Done();

    virtual BOOL SendPacket(PBYTE Data, DWORD Length, HANDLE SyncHandle = NULL);

private:
    static QUIC_STATUS QUIC_API ClientConnectionCallback(
        _In_ HQUIC Connection,
        _In_opt_ void* Context,
        _Inout_ QUIC_CONNECTION_EVENT* Event
        );

    static QUIC_STATUS QUIC_API ClientStreamCallback(
        _In_ HQUIC Stream,
        _In_opt_ void* Context,
        _Inout_ QUIC_STREAM_EVENT* Event
        );

    void InitializeConnection(HQUIC Connection);

    CHAR* m_szIPAddress;
    WORD  m_wDstPort;

    QUIC_BUFFER m_stKeyword;

    VOID LoadConfiguration();

    HQUIC m_hConnection;
    HQUIC m_hStream;

    HANDLE m_hConnectOK;
};


#endif