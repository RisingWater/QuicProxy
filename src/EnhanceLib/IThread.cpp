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
#include "IThread.h"
#include "Thread.h"

IThread* WINAPI CreateIThreadInstance(ThreadMainProc Func, LPVOID Param)
{
    return new CThread(Func, Param);
}

IThread* WINAPI CreateIThreadInstanceEx(ThreadMainProc MainFunc, LPVOID MainParam, ThreadEndProc EndFunc, LPVOID EndParam)
{
    return new CThread(MainFunc, MainParam, EndFunc, EndParam);
}
