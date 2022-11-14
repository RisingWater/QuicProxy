#pragma warning(disable:4996)
#include <stdarg.h>
#include "log.h"
#include "LogEx.h"

TCHAR gModuleNameArray[MAX_PATH] = {'\0'};
static TCHAR* gModuleName = gModuleNameArray;

CLogWriter gLogWriter;

void WINAPI LogEx_Init(TCHAR* ConfigFullPath, DWORD level)
{
    gLogWriter.LogInit(level, ConfigFullPath);
}

void WINAPI LogEx_SetModuleName(TCHAR* ModuleName)
{
    _tcscpy(gModuleNameArray, ModuleName);
}

const TCHAR* WINAPI LogEx_GetModuleName()
{
    return gModuleName;
}

void WINAPI LogEx_Done()
{
    gLogWriter.LogClose();
}

void WINAPI LogEx_Dump(unsigned char* Buffer, unsigned int Length)
{
    unsigned char* p = Buffer;
    unsigned char tmp[16];
    L_TRACE(_T("---------------------------------------------------\n"));
    for (unsigned int i = 0; i < Length / 16; i++)
    {
        L_TRACE(_T("%02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x\n"),
            p[0], p[1], p[2], p[3], p[4], p[5], p[6], p[7], p[8], p[9], p[10], p[11], p[12], p[13], p[14], p[15]);
        p += 16;
    }

    memset(tmp, 0, 16);
    memcpy(tmp, p, Length % 16);
    p = tmp;
    L_TRACE(_T("%02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x\n"),
            p[0], p[1], p[2], p[3], p[4], p[5], p[6], p[7], p[8], p[9], p[10], p[11], p[12], p[13], p[14], p[15]);
    L_TRACE(_T("---------------------------------------------------\n"));
}

BOOL WINAPI LogEx_Printf(DWORD loglevel,
    const TCHAR* ModuleName,
    const CHAR* FunctionName,
    const CHAR* FileName,
    int line,
    TCHAR* logformat, ...)
{
    TCHAR LogBuffer[2048];
    ZeroMemory(LogBuffer, 2048);

    TCHAR* star = LogBuffer;

    va_list args;
    va_start(args, logformat);
#ifdef UNICODE
    DWORD Size = _vsntprintf(star, 2048, logformat, args);
#else
    DWORD Size = vsnprintf(star, 2048, logformat, args);
#endif
    va_end(args);

    gLogWriter.Log(loglevel, ModuleName, FunctionName, FileName, line, star, Size);
    return TRUE;
}
