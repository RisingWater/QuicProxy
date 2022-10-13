#include "stdafx.h"
#include "XGetOpt.h"
#include "enet/enet.h"

typedef enum
{
    TCP_SERVER = 0,
    TCP_CLIENT,
    UDP_SERVER,
    UDP_CLIENT,
    ENET_SERVER,
    ENET_CLIENT,
    QUIC_SERVER,
    QUIC_CLIENT,
    ROLE_TYPE_MAX
} ROLE_TYPE;

const char* roleTypeName[] = {
    "tcpserver",
    "tcpclient",
    "udpserver",
    "udpclient",
    "enetserver",
    "enetclient",
    "quicserver",
    "quicclient"
};

extern void TCPServerTest(int port);
extern void TCPClientTest(char* address, int port);

extern void UDPServerTest(int port);
extern void UDPClientTest(char* address, int port);

extern void ENetServerTest(int port);
extern void ENetClientTest(char* address, int port);

extern void QUICServerTest(int port);
extern void QUICClientTest(char* address, int port);

HANDLE    g_StopEvent  = NULL;
DWORD     g_RecvBytes  = 0;

CRITICAL_SECTION g_RecvByetLock;

DWORD WINAPI TimerProc(void* pParam)
{
    while (TRUE)
    {
        EnterCriticalSection(&g_RecvByetLock);

        DBG_INFO("Transfor Speed %d KBps\r\n", g_RecvBytes / 1024);

        g_RecvBytes = 0;

        LeaveCriticalSection(&g_RecvByetLock);

        DWORD Ret = WaitForSingleObject(g_StopEvent, 1000);
        if (Ret != WAIT_TIMEOUT)
        {
            break;
        }
    }

    return 0;
}

int main(int argc,char * argv[])
{
	int c;

    char ipAddr[64] = { 0 };
    int port = 0;
    ROLE_TYPE roleType = ROLE_TYPE_MAX;

	BOOL isServer = FALSE;

    InitializeCriticalSection(&g_RecvByetLock);

    g_StopEvent = CreateEvent(NULL, TRUE, FALSE, NULL);

    enet_initialize();

	while ((c = getopt(argc, argv, _T("t:a:p:"))) != -1)
    {
        switch (c)
        {
            case _T('t'):
    			DBG_INFO("name: %s\n", optarg);

                for (int i = 0; i < ROLE_TYPE_MAX; i++)
                {
                    if (strcmp(roleTypeName[i], optarg) == 0)
                    {
                        roleType = (ROLE_TYPE)i;
                        break;
                    }
                }

                if (roleType == ROLE_TYPE_MAX)
                {
                    DBG_ERROR("ERROR: unknow role %s\n", optarg);
                    return -1;
                }

                if (roleType == TCP_SERVER || roleType == UDP_SERVER || roleType == ENET_SERVER || roleType == QUIC_SERVER)
                {
                    isServer = TRUE;
                }

	            break;

            case _T('p'):
				DBG_INFO("port: %d\n", atoi(optarg));
                port = atoi(optarg);
                break;

            case _T('a'):
				if (!isServer)
				{
					DBG_INFO("addr: %s\n", optarg);
                    strcpy(ipAddr, optarg);
				}
                break;

            default:
                DBG_ERROR("WARNING: no handler for option %c\n", c);
                return -1;
        }
    }

    if (port == 0)
    {
        DBG_ERROR("ERROR: unknow port\n");
        return -1;
    }

    if (!isServer && strlen(ipAddr) == 0)
    {
        DBG_ERROR("ERROR: unknow addr\n");
        return -1;
    }

    if (isServer)
    {
        CreateThread(NULL, 0, TimerProc, NULL, 0, NULL);
    }

    switch (roleType)
    {
        case TCP_SERVER:
            TCPServerTest(port);
            break;
        case TCP_CLIENT:
            TCPClientTest(ipAddr, port);
            break;
        case UDP_SERVER:
            UDPServerTest(port);
            break;
        case UDP_CLIENT:
            UDPClientTest(ipAddr, port);
            break;
        case ENET_SERVER:
            ENetServerTest(port);
            break;
        case ENET_CLIENT:
            ENetClientTest(ipAddr, port);
            break;
        case QUIC_SERVER:
            QUICServerTest(port);
            break;
        case QUIC_CLIENT:
            QUICClientTest(ipAddr, port);
            break;
        default:
            DBG_ERROR("ERROR: unknow type\n");
            break;
    }

    enet_deinitialize();

    DeleteCriticalSection(&g_RecvByetLock);
}


