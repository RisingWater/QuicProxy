#ifndef __IOPINI_H__
#define __IOPINI_H__

#ifdef WIN32
#include <Windows.h>
#else
#include <winpr/wtypes.h>
#endif

#include "BaseObject.h"

class IFileIni : public virtual CBaseObject
{
public:
    virtual BOOL ReadOption(const CHAR* segment, const CHAR* name, CHAR* value) = 0;

    virtual BOOL ReadOption(const CHAR* segment, const CHAR* name, DWORD* value) = 0;

    virtual BOOL DeleteOption(const CHAR* segment, const CHAR* name) = 0;

    virtual BOOL WriteOption(const CHAR* segment, const CHAR* name, CHAR* value) = 0;

    virtual BOOL WriteOption(const CHAR* segment, const CHAR* name, DWORD value) = 0;
};

extern "C"
{
    IFileIni* WINAPI CreateIFileIniInstance(const CHAR* inifile);
}

#endif
