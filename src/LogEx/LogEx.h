#pragma once

#ifndef __LOG_EX_BASE_H__
#define __LOG_EX_BASE_H__

#ifdef WIN32
#include <Windows.h>
#include <tchar.h>
#else
#include <winpr/wtypes.h>
#include <winpr/tchar.h>
#endif

#ifndef MODULE_NAME
#define MODULE_NAME (LogEx_GetModuleName())
#endif

#ifdef WIN32
#define DLL_LOGEX_LIB_API __declspec(dllexport)
#else
#define DLL_LOGEX_LIB_API __attribute__ ((visibility("default")))
#endif

#define LOG_LEVEL_DEBUG   1
#define LOG_LEVEL_TRACE   2
#define LOG_LEVEL_INFO    3
#define LOG_LEVEL_WARNING 4
#define LOG_LEVEL_ERROR   5

extern "C"
{
    DLL_LOGEX_LIB_API void WINAPI LogEx_Init(TCHAR* LogPath, DWORD Level);

    DLL_LOGEX_LIB_API void WINAPI LogEx_Dump(unsigned char* Buffer, unsigned int Length);

    DLL_LOGEX_LIB_API void WINAPI LogEx_SetModuleName(TCHAR* ModuleName);

    DLL_LOGEX_LIB_API const TCHAR* WINAPI LogEx_GetModuleName();

    DLL_LOGEX_LIB_API BOOL WINAPI LogEx_Printf(DWORD loglevel,
        const TCHAR* ModuleName,
        const CHAR* FunctionName,
        const CHAR* FileName,
        int line,
        TCHAR* logformat, ...);

    DLL_LOGEX_LIB_API void WINAPI LogEx_Done();
}

#define L_TRACE(...)                                                                                   \
        LogEx_Printf(LOG_LEVEL_TRACE, MODULE_NAME, __FUNCTION__, __FILE__, __LINE__, __VA_ARGS__);   \

#define L_DEBUG(...)                                                                           \
        LogEx_Printf(LOG_LEVEL_DEBUG, MODULE_NAME, __FUNCTION__, __FILE__, __LINE__, __VA_ARGS__);   \

#define L_INFO(...)                                                                            \
        LogEx_Printf(LOG_LEVEL_INFO, MODULE_NAME, __FUNCTION__, __FILE__, __LINE__, __VA_ARGS__);    \

#define L_WARN(...)                                                                            \
        LogEx_Printf(LOG_LEVEL_WARNING, MODULE_NAME, __FUNCTION__, __FILE__, __LINE__, __VA_ARGS__); \

#define L_ERROR(...)                                                                           \
        LogEx_Printf(LOG_LEVEL_ERROR, MODULE_NAME,  __FUNCTION__, __FILE__, __LINE__, __VA_ARGS__);  \

#define L_DUMP(Buffer, Length)                                                               \
        LogEx_Printf((unsigned char*)Buffer, (unsigned int)Length);                                    \

#define L_TRACE_ENTER() L_TRACE(("Enter\n"))
#define L_TRACE_LEAVE() L_TRACE(("Leave\n"))

#endif
