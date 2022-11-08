#pragma once

#ifndef __THREAD_H__
#define __THREAD_H__

#include "IThread.h"
#include "BaseObject.h"

class CThread : public IThread
{
public:
    CThread(ThreadMainProc Func, LPVOID Param);
    CThread(ThreadMainProc MainFunc, LPVOID MainParam, ThreadEndProc Endfunc, LPVOID Endparam);

    virtual ~CThread(void);
public:
    virtual void WINAPI StartMainThread();
    virtual void WINAPI StopMainThread();
    virtual BOOL WINAPI IsMainThreadRunning();

private:
    void Init(ThreadMainProc MainFunc, LPVOID MainParam, ThreadEndProc Endfunc, LPVOID Endparam);

    static DWORD WINAPI MainThread(LPVOID Lp);

    HANDLE           m_hMainThread;
    DWORD            m_dwMainThreadId;
    HANDLE           m_hMainThreadStartedEvent;
    HANDLE           m_hStopMainThreadEvent;
    HANDLE           m_hMainThreadStopedEvent;
    BOOL             m_bContinueMainThread;

    ThreadMainProc   m_fnMainProc;
    LPVOID           m_pMainProcParam;
    ThreadEndProc    m_fnEndProc;
    LPVOID           m_pEndProcParam;
    CRITICAL_SECTION m_csEndLock;
};

#ifndef WIN32
#if defined __cplusplus
extern "C" {
#endif

    void winpr_get_version(int* major, int* minor, int* revision)  __attribute__((weak)) ;

#if defined __cplusplus
}
#endif
#endif

#endif
