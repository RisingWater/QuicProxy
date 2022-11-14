#pragma once

#ifndef __ENHANCE_LOG_H__
#define __ENHANCE_LOG_H__

#ifdef WIN32
#include <Windows.h>
#else
#include <winpr/wtypes.h>
#include <winpr/tchar.h>
#include <winpr/interlocked.h>
#include <winpr/file.h>
#include <winpr/synch.h>
#include <winpr/thread.h>
#include <sys/errno.h>
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "BaseObject.h"

#define LOG_BUFFSIZE   (1024 * 1024 * 4)
#define LOG_MODULE_LEN (32)

class CLogWriter : public CBaseObject
{
public:
    CLogWriter();
    virtual ~CLogWriter();

    BOOL LogInit(DWORD loglevel, const TCHAR* LogPath, BOOL append = TRUE);

    BOOL Log(DWORD loglevel,
        const TCHAR* ModuleName,
        const CHAR* FunctionName,
        const CHAR* FileName,
        int line,
        TCHAR* logmessage,
        DWORD Size);

    BOOL LogClose();
private:
    const CHAR* LogLevelToString(DWORD loglevel);

    BOOL CheckLevel(DWORD loglevel);

    int PremakeString(CHAR* pBuffer,
        const TCHAR* ModuleName,
        const CHAR* FunctionName,
        const CHAR* FileName,
        int line,
        DWORD loglevel);

    BOOL WriteLog(CHAR* pBuffer, int len);

    DWORD    m_dwLogLevel;
    FILE*    m_hFileHandle;
    BOOL     m_bIsAppend;
    TCHAR    m_szLogPath[MAX_PATH];
    BOOL     m_bToDbgPort;

    CRITICAL_SECTION m_csLock;
    CHAR     m_pBuffer[LOG_BUFFSIZE];
};

#endif
