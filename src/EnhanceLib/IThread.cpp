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
