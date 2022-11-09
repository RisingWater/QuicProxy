#pragma once

#ifndef __STDAFX_H__
#define __STDAFX_H__

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#ifdef WIN32
#include <WinSock2.h>
#include <windows.h>
#include <ws2tcpip.h>
#include <tchar.h>
#pragma warning(disable:4127)
#pragma warning(disable:4200)
#pragma warning(disable:4996)
#else
#include <errno.h>
#include "winpr/wtypes.h"
#include "winpr/tchar.h"
#include "winpr/file.h"
#include "winpr/synch.h"
#include "winpr/thread.h"
#include "winpr/interlocked.h"
#endif

#define DBG_ERROR _tprintf
#define DBG_WARN  _tprintf
#define DBG_INFO  _tprintf
#define DBG_TRACE _tprintf

#endif