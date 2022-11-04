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

#define DBG_ERROR printf
#define DBG_WARN  printf
#define DBG_INFO  printf
#define DBG_TRACE printf

#endif