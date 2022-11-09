#pragma once

#ifndef __ITHREAD_H__
#define __ITHREAD_H__

#ifdef WIN32
#include <Windows.h>
#include <list>
#else
#include <winpr/wtypes.h>
#include <winpr/synch.h>
#endif

#include "BaseObject.h"

typedef BOOL(*ThreadMainProc)(LPVOID param, HANDLE stopevent);

typedef void(*ThreadEndProc)(LPVOID param);

class IThread : public virtual CBaseObject
{
public:
    virtual void WINAPI StartMainThread() = 0;

    virtual void WINAPI StopMainThread() = 0;

    virtual BOOL WINAPI IsMainThreadRunning() = 0;
};

extern "C"
{
    IThread* WINAPI CreateIThreadInstance(ThreadMainProc Func, LPVOID Param);
 
    IThread* WINAPI CreateIThreadInstanceEx(ThreadMainProc MainFunc, LPVOID MainParam, ThreadEndProc Endfunc, LPVOID Endparam);
}

#endif
