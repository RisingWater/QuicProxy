#pragma once

#ifndef __PKT_IBUFFER_H__
#define __PKT_IBUFFER_H__

#ifdef WIN32
#include <Windows.h>
#else
#include <winpr/wtypes.h>
#endif

#include "BaseObject.h"

#define PACKET_BUFFER_HEADROOM_DEFAULT 1024
#define PACKET_BUFFER_TAILROOM_DEFAULT 1024

class IPacketBuffer : public virtual CBaseObject
{
public:
    virtual BOOL WINAPI DataPush(DWORD len) = 0;

    virtual BOOL WINAPI DataPull(DWORD len) = 0;

    virtual BOOL WINAPI TailPush(DWORD len) = 0;

    virtual BOOL WINAPI TailPull(DWORD len) = 0;

    virtual BYTE* WINAPI GetData() = 0;

    virtual BYTE* WINAPI GetTail() = 0;

    virtual DWORD WINAPI GetBufferLength() = 0;
};

extern "C"
{
    IPacketBuffer* WINAPI CreateIBufferInstance(DWORD len);

    IPacketBuffer* WINAPI CreateIBufferInstanceEx(BYTE* buffer, DWORD len);

    IPacketBuffer* WINAPI CreateIBufferInstanceEx2(BYTE* buffer, DWORD len, DWORD HeadRoom, DWORD TailRoom);
}

#endif