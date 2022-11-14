#pragma once

#ifndef __QUIC_BASE_H__
#define __QUIC_BASE_H__

#include "msquic.h"
#include "BaseObject.h"
#include "QUICCommLib.h"
#include <list>

typedef enum
{
    QUICBASE_LOW_LATENCY = 0,
    QUICBASE_HIGH_THOUGHPUT,
    QUICBASE_BACKGROUND,
    QUICBASE_REALTIME,
} QUICBASE_TYPE;

class CQUICChannel;
class CQUICCtrlChannel;

class CQUICBase
{
public:
    CQUICBase();
    
    virtual ~CQUICBase();
    
protected:
    virtual BOOL BaseInit(QUICBASE_TYPE Type);
    virtual VOID BaseDone();

    const QUIC_API_TABLE* m_pMsQuic;
    QUIC_REGISTRATION_CONFIG m_stRegConfig;
    HQUIC m_hRegistration;
    HQUIC m_hConfiguration;
};

class CQUICLink : public virtual IQUICLink
{
public:
    CQUICLink();

    virtual ~CQUICLink();

    virtual BOOL SendCtrlPacket(PBYTE Data, DWORD DataLen);

    virtual PBYTE RecvCtrlPacket(DWORD* DataLen, DWORD TimeOut);

    virtual void FreeRecvedCtrlPacket(PBYTE Data);

    virtual void DestoryChannel(IQUICChannel* channel);

    virtual VOID RegisterEndProcess(_QUICLinkDisconnectedProcess Process, CBaseObject* Param);

protected:
    void LinkDisconnectNotify();

    void LinkDone();

    CRITICAL_SECTION m_csLock;

    std::list<CQUICChannel*> m_Streams;

    CQUICCtrlChannel* m_pCtrlChannel;

    _QUICLinkDisconnectedProcess m_pfnDisconnectProcess;

    CBaseObject* m_pParam;
};

#endif