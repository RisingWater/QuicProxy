#ifndef __OPINI_H__
#define __OPINI_H__

#include "IOpini.h"
#include "BaseObject.h"
#include <stdio.h>

class CFileIni : public IFileIni
{
public:
    CFileIni(const CHAR* IniFile);

    virtual ~CFileIni();

    virtual BOOL ReadOption(const CHAR* Segment, const CHAR* Name, CHAR* Value);

    virtual BOOL ReadOption(const CHAR* Segment, const CHAR* Name, DWORD* Value);

    virtual BOOL DeleteOption(const CHAR* Segment, const CHAR* Name);

    virtual BOOL WriteOption(const CHAR* Segment, const CHAR* Name, CHAR* Value);

    virtual BOOL WriteOption(const CHAR* Segment, const CHAR* Name, DWORD Value);

private:
    BOOL ReadLineFromFile(FILE *fp, CHAR *Data);

    BOOL ReadAllBufferFromFile(CHAR *Data, DWORD Length);

    BOOL WriteAllBufferToFile(const CHAR *Data);

    void InsertLineFromBuffer(CHAR *sp, CHAR *line);

    void DeleteLineFromBuffer(CHAR *sp);

    CHAR *GetLineFromBuffer(CHAR *sp, CHAR *dest, DWORD maxlen);

    CHAR* m_szIniFileName;
};


#endif
