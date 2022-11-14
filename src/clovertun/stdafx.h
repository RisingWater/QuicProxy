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

#include "LogEx.h"

#define DBG_ERROR L_ERROR
#define DBG_WARN  L_WARN
#define DBG_INFO  L_INFO
#define DBG_TRACE L_TRACE

#endif