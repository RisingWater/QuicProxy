#include "stdafx.h"
#include "QUICCommLib.h"
#include "SocketProxy.h"
#include "ProxyProcessor.h"
#include "IOpini.h"

#define CERT_PATH "ca.crt"
#define KEY_PATH  "ca.key"

HANDLE g_StopEvent;

typedef enum
{
    PROXY_SERVER = 0,
    PROXY_CLIENT,
    ROLE_TYPE_MAX
} PROXY_ROLE_TYPE;

const CHAR* roleTypeName[] = {
    "proxyserver",
    "proxyclient",
    "proxymax"
};

static BOOL GetFullPathRelative(const CHAR *filename, CHAR* filePath, DWORD filePathSize)
{
    CHAR tmp[MAX_PATH] = { 0 };
    CHAR *p;

    if (filename == NULL || filePath == NULL)
    {
        return FALSE;
    }

    memset(tmp, 0, filePathSize);
#ifdef WIN32    
    GetModuleFileNameA(NULL, tmp, MAX_PATH);
#else
    strcpy(tmp, getenv("HOME"));
#endif

    p = strrchr(tmp, _T('\\'));
    if (p != NULL)
    {
        *(p + 1) = NULL;

        int cch = strlen(tmp) + strlen(filename) + 1;
        DWORD size = cch * sizeof(CHAR);

        if (size > filePathSize || cch > MAX_PATH)
        {
            return FALSE;
        }

        strcat(tmp, filename);

#ifdef WIN32
        DWORD ret = GetFullPathNameA(tmp, MAX_PATH, filePath, NULL);

        if (ret > 0 && ret <= MAX_PATH)
        {
            return TRUE;
        }
#else
        strcpy(filePath, tmp);
        return TRUE;
#endif

    }

    return FALSE;
}

PROXY_ROLE_TYPE GetRoleType(CHAR* szConfigPath)
{
    CHAR szRole[MAX_PATH] = { 0 };
    PROXY_ROLE_TYPE roleType = ROLE_TYPE_MAX;

    IFileIni* Ini = CreateIFileIniInstance(szConfigPath);

    if (Ini->ReadOption("config", "role", szRole))
    {
        for (int i = 0; i < ROLE_TYPE_MAX; i++)
        {
            if (strcmp(roleTypeName[i], szRole) == 0)
            {
                roleType = (PROXY_ROLE_TYPE)i;
                break;
            }
        }
    }

    Ini->Release();

    return roleType;
}

BOOL GetAddressAndPort(CHAR* szConfigPath, CHAR* pAddress, DWORD dwAddressSize, int* pdwPort)
{
    CHAR szProxyConfig[MAX_PATH] = { 0 };

    IFileIni* Ini = CreateIFileIniInstance(szConfigPath);

    if (Ini->ReadOption("config", "proxy_server", szProxyConfig))
    {
        CHAR *p;
        p = strchr(szProxyConfig, _T(':'));
        if (p != NULL)
        {
            *pdwPort = strtol(p + 1, NULL, 10);
            *p = 0;
        }

        if (pAddress)
        {
            strncpy(pAddress, szProxyConfig, dwAddressSize);
        }

        return TRUE;
    }

    Ini->Release();

    return FALSE;
}

BOOL IsServer(PROXY_ROLE_TYPE roleType)
{
    return roleType == PROXY_SERVER;
}

void ProxyServer(int port)
{
    IQUICService* pService = NULL;

    CHAR CertPath[MAX_PATH] = { 0 };
    CHAR KeyPath[MAX_PATH] = { 0 };

    if (!GetFullPathRelative(CERT_PATH, CertPath, MAX_PATH))
    {
        DBG_ERROR(_T("get CertPath failed\r\n"));
        return;
    }

    if (!GetFullPathRelative(KEY_PATH, KeyPath, MAX_PATH))
    {
        DBG_ERROR(_T("get CertPath failed\r\n"));
        return;
    }

    pService = CreateIQUICService(port, "xredtest", CertPath, KeyPath);
    if (!pService->StartListen())
    {
        DBG_ERROR(_T("StartListen Failed\r\n"));
        return;
    }

    while (TRUE)
    {
        IQUICServer* pServer = pService->Accept(g_StopEvent);

        if (pServer == NULL)
        {
            break;
        }

        CSocketProxy* pProxy = new CSocketProxy(pServer);
        pProxy->Init();
    }

    pService->StopListen();
    pService->Release();

    return;
}

void ProxyClient(CHAR* ConfigPath, CHAR* address, int port)
{
    CProxyProcessor* Processor = new CProxyProcessor(g_StopEvent, address, port);
    if (Processor->Init(ConfigPath))
    {
        Processor->Run();
        Processor->Done();
    }

    Processor->Release();
}

int main(int argc,char * argv[])
{
    g_StopEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
    CHAR szConfigPath[MAX_PATH] = { 0 };

    if (argc <= 1)
    {
        if (!GetFullPathRelative("QuicProxyConf.ini", szConfigPath, MAX_PATH))
        {
            DBG_ERROR(_T("get config path failed\r\n"));
            return -1;
        }
    }
    else
    {
        strcpy(szConfigPath, argv[1]);
    }

    PROXY_ROLE_TYPE roleType = GetRoleType(szConfigPath);

    if (roleType == ROLE_TYPE_MAX)
    {
        DBG_ERROR(_T("get role failed\r\n"));
        return -1;
    }

    DBG_ERROR(_T("fuck1\n"));

    CHAR ipAddr[64] = { 0 };
    int port = 0;

    if (!GetAddressAndPort(szConfigPath, ipAddr, 64, &port))
    {
        DBG_ERROR(_T("get config address and port failed\r\n"));
        return -1;
    }

    if (port == 0)
    {
        DBG_ERROR(_T("ERROR: unknow port\n"));
        return -1;
    }

    DBG_ERROR(_T("fuck1\r\n"));

    if (IsServer(roleType))
    {
        ProxyServer(port);
    }
    else
    {
        if (strlen(ipAddr) == 0)
        {
            DBG_ERROR(_T("ERROR: unknow addr\n"));
            return -1;
        }

        ProxyClient(szConfigPath, ipAddr, port);
    }
}


