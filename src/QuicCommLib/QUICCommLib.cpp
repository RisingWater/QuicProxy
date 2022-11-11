#include "stdafx.h"
#include "QUICCommLib.h"
#include "QUICService.h"
#include "QUICClient.h"

IQUICService* WINAPI CreateIQUICService(WORD Port, const CHAR* Keyword, const CHAR* CertFilePath, const CHAR* KeyFilePath)
{
    return new CQUICService(Port, Keyword, CertFilePath, KeyFilePath);
}

IQUICClient* WINAPI CreateIQUICClient(CHAR* ipaddr, WORD dstPort, const CHAR* Keyword)
{
    return new CQUICClient(ipaddr, dstPort, Keyword);
}
