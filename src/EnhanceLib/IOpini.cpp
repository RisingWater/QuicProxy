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
#include "IOpini.h"
#include "Opini.h"

IFileIni* WINAPI CreateIFileIniInstance(const CHAR* inifile)
{
    return new CFileIni(inifile);
}