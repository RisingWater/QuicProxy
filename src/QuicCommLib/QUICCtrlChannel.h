#pragma once

#ifndef __QUIC_CTRL_CHANNEL_H__
#define __QUIC_CTRL_CHANNEL_H__

#include "QUICChannel.h"
#include "BaseObject.h"

class CQUICCtrlChannel : public CBaseObject
{
public:
    CQUICCtrlChannel();

    virtual ~CQUICCtrlChannel();

    BOOL Init(const QUIC_API_TABLE* pMsQuic, HQUIC hConnection);

    BOOL Init(const QUIC_API_TABLE* pMsQuic, HQUIC Stream, HQUIC hConnection);

    void Done();

    BOOL SendCtrlPacket(PBYTE Data, DWORD DataLen);

    PBYTE RecvCtrlPacket(DWORD* DataLen, DWORD TimeOut);

    void FreeRecvedCtrlPacket(PBYTE Data);

private:
    static BOOL QUICtrlProcess(PBYTE Data, DWORD Length, IQUICChannel* quic, CBaseObject* Param);
    
    CQUICChannel* m_pCtrlStream;

    std::list<QUIC_BUFFER> m_CtrlPktList;

    CRITICAL_SECTION m_csPktLock;

    HANDLE m_hPktRecved;
};


#endif