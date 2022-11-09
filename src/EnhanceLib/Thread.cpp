#ifdef WIN32
#include <windows.h>
#include <tchar.h>
#else
#include "winpr/wtypes.h"
#include "winpr/tchar.h"
#include "winpr/file.h"
#include "winpr/synch.h"
#include "winpr/thread.h"
#include "winpr/interlocked.h"
#endif
#include "Thread.h"

#define MODULE_NAME _T("CThread")
#ifndef WIN32
#include <pthread.h>
#endif

#define INVALID_THREAD_ID ((DWORD)-1)



CThread::CThread(ThreadMainProc Func, LPVOID Param) : CBaseObject()
{
    Init(Func, Param, NULL, NULL);
}

CThread::CThread(ThreadMainProc MainFunc, LPVOID MainParam, ThreadEndProc EndFunc, LPVOID EndParam) : CBaseObject()
{
    Init(MainFunc, MainParam, EndFunc, EndParam);
}

void CThread::Init(ThreadMainProc MainFunc, LPVOID MainParam, ThreadEndProc EndFunc, LPVOID EndParam)
{
    m_hMainThread = NULL;
    m_dwMainThreadId = INVALID_THREAD_ID;

    m_hStopMainThreadEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
    m_hMainThreadStopedEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
    m_hMainThreadStartedEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
    SetEvent(m_hMainThreadStopedEvent);

    m_bContinueMainThread = FALSE;

    m_fnMainProc = MainFunc;
    m_pMainProcParam = MainParam;

    m_fnEndProc = EndFunc;
    m_pEndProcParam = EndParam;
    InitializeCriticalSection(&m_csEndLock);
}

CThread::~CThread(void)
{
    StopMainThread();

    if (m_hStopMainThreadEvent)
    {
        CloseHandle(m_hStopMainThreadEvent);
        m_hStopMainThreadEvent = NULL;
    }

    if (m_hMainThreadStopedEvent)
    {
        CloseHandle(m_hMainThreadStopedEvent);
        m_hMainThreadStopedEvent = NULL;
    }

    if (m_hMainThreadStartedEvent)
    {
        CloseHandle(m_hMainThreadStartedEvent);
        m_hMainThreadStopedEvent = NULL;
    }

    if (m_hMainThread)
    {
        CloseHandle(m_hMainThread);
        m_hMainThread = NULL;
        m_dwMainThreadId = INVALID_THREAD_ID;
    }
    DeleteCriticalSection(&m_csEndLock);
}

void CThread::StartMainThread()
{
	DWORD dwRet;
    ResetEvent(m_hStopMainThreadEvent);
    ResetEvent(m_hMainThreadStartedEvent);
   
    m_hMainThread = CreateThread(NULL, 0, CThread::MainThread, this, 0, &m_dwMainThreadId);

	dwRet = WaitForSingleObject(m_hMainThreadStartedEvent, 5000);
}

void CThread::StopMainThread()
{
    //如果线程不在运行，则回收句柄之后返回，以保证可以再次启动
    if (!IsMainThreadRunning())
    {
        if (m_hMainThread)
        {
            CloseHandle(m_hMainThread);
            m_hMainThread = NULL;
            m_dwMainThreadId = INVALID_THREAD_ID;
        }
        return;
    }

    if (m_hMainThread == NULL)
    {
        return;
    }

    //先破除循环
    m_bContinueMainThread = FALSE;
    //再置事件
    SetEvent(m_hStopMainThreadEvent);

}

DWORD WINAPI CThread::MainThread(LPVOID Lp)
{
    CThread* Thread = (CThread*)Lp;

	if (Thread)
	{
		Thread->AddRef();
	}
	else
	{
		return 0;
	}

    //通知线程已经启动
    ResetEvent(Thread->m_hStopMainThreadEvent);
    ResetEvent(Thread->m_hMainThreadStopedEvent);
    Thread->m_bContinueMainThread = TRUE;
    SetEvent(Thread->m_hMainThreadStartedEvent);

    //主线程循环执行
    while (Thread->m_bContinueMainThread)
    {
        if (!Thread->m_fnMainProc(Thread->m_pMainProcParam, Thread->m_hStopMainThreadEvent))
        {
            //返回false就退出
            break;
        }
    }
    //结束函数调用
    EnterCriticalSection(&Thread->m_csEndLock);
    if (Thread->m_fnEndProc)
    {
        Thread->m_fnEndProc(Thread->m_pEndProcParam);
    }
    //设置结束事件
    SetEvent(Thread->m_hMainThreadStopedEvent);
    LeaveCriticalSection(&Thread->m_csEndLock);

    Thread->Release();
    return 0;
}

BOOL CThread::IsMainThreadRunning()
{
    //检测stopevent是否有被置位，以此判断线程是否在运行
    DWORD ret = WaitForSingleObject(m_hMainThreadStopedEvent, 0);
    if (ret == WAIT_TIMEOUT)
    {
        return TRUE;
    }
    return FALSE;
}



