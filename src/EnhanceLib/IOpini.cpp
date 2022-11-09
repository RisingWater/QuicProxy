#include "IOpini.h"
#include "Opini.h"

IFileIni* WINAPI CreateIFileIniInstance(const CHAR* inifile)
{
    return new CFileIni(inifile);
}