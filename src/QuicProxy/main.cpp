#include "stdafx.h"

typedef enum
{
    PROXY_SERVER = 0,
    PROXY_CLIENT,
    ROLE_TYPE_MAX
} PROXY_ROLE_TYPE;

const TCHAR* roleTypeName[] = {
    _T("proxyerver"),
    _T("proxyclient"),
    _T("proxymax")
};

static BOOL GetFullPathRelative(const TCHAR *filename, TCHAR* filePath, DWORD filePathSize)
{
    TCHAR tmp[MAX_PATH] = { 0 };
    TCHAR *p;

    if (filename == NULL || filePath == NULL)
    {
        return FALSE;
    }

    memset(tmp, 0, filePathSize);
    GetModuleFileName(NULL, tmp, MAX_PATH);

    p = _tcsrchr(tmp, _T('\\'));
    if (p != NULL)
    {
        *(p + 1) = NULL;

        int cch = _tcslen(tmp) + _tcslen(filename) + 1;
        DWORD size = cch * sizeof(TCHAR);

        if (size > filePathSize || cch > MAX_PATH)
        {
            return FALSE;
        }

        _tcscat(tmp, filename);

        DWORD ret = GetFullPathName(tmp, MAX_PATH, filePath, NULL);
        if (ret > 0 && ret <= MAX_PATH)
        {
            return TRUE;
        }
    }

    return FALSE;
}

PROXY_ROLE_TYPE GetRoleType(TCHAR* szConfigPath)
{
    TCHAR szRole[MAX_PATH] = { 0 };
    PROXY_ROLE_TYPE roleType = ROLE_TYPE_MAX;

    if (GetPrivateProfileString(_T("config"), _T("role"), _T(""), szRole, MAX_PATH, szConfigPath))
    {
        for (int i = 0; i < ROLE_TYPE_MAX; i++)
        {
            if (_tcscmp(roleTypeName[i], szRole) == 0)
            {
                roleType = (PROXY_ROLE_TYPE)i;
                break;
            }
        }
    }

    return roleType;
}

BOOL GetAddressAndPort(TCHAR* szConfigPath, TCHAR* pAddress, DWORD dwAddressSize, int* pdwPort)
{
    TCHAR szProxyConfig[MAX_PATH] = { 0 };

    if (GetPrivateProfileString("config", "proxy_server", "", szProxyConfig, MAX_PATH, szConfigPath))
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
            _tcsncpy(pAddress, szProxyConfig, dwAddressSize);
        }

        return TRUE;
    }

    return FALSE;
}

BOOL IsServer(PROXY_ROLE_TYPE roleType)
{
    return roleType == PROXY_SERVER;
}

int main(int argc,char * argv[])
{
    TCHAR szConfigPath[MAX_PATH] = { 0 };
    if (!GetFullPathRelative("QuicProxyConf.ini", szConfigPath, MAX_PATH))
    {
        DBG_ERROR("get config path failed\r\n");
        return -1;
    }

    PROXY_ROLE_TYPE roleType = GetRoleType(szConfigPath);

    char ipAddr[64] = { 0 };
    int port = 0;

    if (GetAddressAndPort(szConfigPath, ipAddr, 64, &port))
    {
        DBG_ERROR("get config address and port failed\r\n");
        return -1;
    }

    if (port == 0)
    {
        DBG_ERROR("ERROR: unknow port\n");
        return -1;
    }
    
    if (IsServer(roleType))
    {
        
    }
    else
    {
        if (strlen(ipAddr) == 0)
        {
            DBG_ERROR("ERROR: unknow addr\n");
            return -1;
        }
    }
}


