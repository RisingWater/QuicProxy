#pragma warning(disable:4996)

#include <time.h>
#include "LogEx.h"
#include "log.h"

CLogWriter::CLogWriter() : CBaseObject()
{
    m_dwLogLevel = LOG_LEVEL_INFO;
    m_bToDbgPort = FALSE;
    m_hFileHandle = NULL;
    m_bIsAppend = TRUE;
    ZeroMemory(m_szLogPath, MAX_PATH);

    InitializeCriticalSection(&m_csLock);
}

CLogWriter::~CLogWriter()
{
    LogClose();
    DeleteCriticalSection(&m_csLock);
}

const CHAR* CLogWriter::LogLevelToString(DWORD loglevel) {
    switch (loglevel) {
        case LOG_LEVEL_DEBUG:
            return "DEBUG";
        case LOG_LEVEL_TRACE:
            return "TRACE";
        case LOG_LEVEL_INFO:
            return "NOTICE";
        case LOG_LEVEL_WARNING:
            return "WARN";
        case LOG_LEVEL_ERROR:
            return "ERROR";
        default:
            return "UNKNOWN";
    }
}

BOOL CLogWriter::CheckLevel(DWORD loglevel)
{
    if (loglevel >= m_dwLogLevel)
        return TRUE;
    else
        return FALSE;
}

BOOL CLogWriter::LogInit(DWORD loglevel, const TCHAR* LogPath, BOOL append)
{
    m_dwLogLevel = loglevel;
    m_bIsAppend = append;
    if (_tcslen(LogPath) >= (MAX_PATH - 1))
    {
        fprintf(stderr, "the path of log file is too long:%d limit:%d\n", _tcslen(LogPath), MAX_PATH);
        return FALSE;
    }

    _tcscpy(m_szLogPath, LogPath);

    if (_tcscmp(m_szLogPath, _T("stdio")) == 0)
    {
        m_hFileHandle = NULL;
        fprintf(stderr, "now all the running-information are going to put to stdio\n");
        return TRUE;
    }

#ifdef WIN32
    if (_tcscmp(m_szLogPath, _T("dbgport")) == 0)
    {
        m_hFileHandle = NULL;
        m_bToDbgPort = TRUE;
        fprintf(stderr, "now all the running-information are going to put to dbgport\n");
        return TRUE;
    }

    m_hFileHandle = _tfopen(m_szLogPath, append ? _T("a") : _T("w"));
#else
    m_hFileHandle = fopen(m_szLogPath, append ? ("a") : ("w"));
#endif

    if (m_hFileHandle == NULL)
    {
        fprintf(stderr, "cannot open log file,file location is %s\n", m_szLogPath);
        return FALSE;
    }

    setvbuf(m_hFileHandle, (char *)NULL, _IOLBF, 0);

    fprintf(stderr, "now all the running-information are going to the file %s\n", m_szLogPath);
    return TRUE;
}

BOOL CLogWriter::Log(DWORD loglevel, 
        const TCHAR* ModuleName,
        const CHAR* FunctionName,
        const CHAR* FileName,
        int line,
        TCHAR* logmessage,
        DWORD Size)
{
    int _size;
    int prestrlen = 0;

    CHAR prefix[2048];
    EnterCriticalSection(&m_csLock);
    prestrlen = PremakeString(prefix, ModuleName, FunctionName, FileName, line, loglevel);

#ifdef UNICODE
    sprintf(m_pBuffer, "%s%S", prefix, logmessage);
#else
    sprintf(m_pBuffer, "%s%s", prefix, logmessage);
#endif
    _size = Size;

    if (NULL == m_hFileHandle)
    {
        if (m_bToDbgPort)
        {
#ifdef WIN32
            OutputDebugStringA(m_pBuffer);
#endif
        }
        else
        {
            fprintf(stdout, "%s", m_pBuffer);
            fflush(stdout);
        }
    }
    else
    {
        WriteLog(m_pBuffer, strlen(m_pBuffer) + 1);
    }

    LeaveCriticalSection(&m_csLock);
    return TRUE;
}

int CLogWriter::PremakeString(CHAR* pBuffer,
        const TCHAR* ModuleName,
        const CHAR* FunctionName,
        const CHAR* FileName,
        int line,
        DWORD loglevel)
{
    time_t now;
    now = time(&now);
    struct tm vtm;
    int ThreadId = GetCurrentThreadId();

    line;
    FileName;

#ifdef WIN32
    localtime_s(&vtm, &now);
    return _snprintf(pBuffer, LOG_BUFFSIZE, "%02d-%02d %02d:%02d:%02d %s(%S) [%s]",
        vtm.tm_mon + 1, vtm.tm_mday, vtm.tm_hour, vtm.tm_min, vtm.tm_sec,
        LogLevelToString(loglevel),
        ModuleName,
        FunctionName);
#else
    localtime_r(&now, &vtm);
    return _snprintf(pBuffer, LOG_BUFFSIZE, "%02d-%02d %02d:%02d:%02d %s(%s) [%s]",
        vtm.tm_mon + 1, vtm.tm_mday, vtm.tm_hour, vtm.tm_min, vtm.tm_sec,
        LogLevelToString(loglevel),
        ModuleName,
        FunctionName);
#endif


}

BOOL CLogWriter::WriteLog(CHAR* pbuffer, int len)
{
    if (1 == fwrite(pbuffer, len, 1, m_hFileHandle)) //only write 1 item
    {
        fflush(m_hFileHandle);
    }
    else
    {
        int x = errno;
        fprintf(stderr, "Failed to write to logfile. errno:%s    message:%s", strerror(x), pbuffer);
        return FALSE;
    }
    return TRUE;
}

BOOL CLogWriter::LogClose()
{
    if (m_hFileHandle == NULL)
    {
        return FALSE;
    }

    fflush(m_hFileHandle);
    fclose(m_hFileHandle);
    m_hFileHandle = NULL;
    return TRUE;
}


