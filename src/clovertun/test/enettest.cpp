#include "stdafx.h"
#include "ENetClient.h"
#include "packetunit.h"

#define DATA_SIZE (64 * 1024)

extern HANDLE g_StopEvent;
extern DWORD  g_RecvBytes;
extern CRITICAL_SECTION g_RecvByetLock;

static BOOL ENetRecvPacketProcess(PBYTE Data, DWORD Length, CENetClient* tcp, CBaseObject* Param)
{
    EnterCriticalSection(&g_RecvByetLock);
    g_RecvBytes += Length;
    LeaveCriticalSection(&g_RecvByetLock);

    tcp->SendPacket((PBYTE)&Length, 4);

    return TRUE;
}

void ENetServerTest(int port)
{
    CENetClient* pENetHost = NULL;

    CLIENT_INFO clientinfo;
    memset(&clientinfo, 0, sizeof(CLIENT_INFO));
    clientinfo.port = port;

    pENetHost = new CENetClient(0, clientinfo, TRUE);
    if (pENetHost)
    {
        pENetHost->RegisterRecvProcess(ENetRecvPacketProcess, NULL);
        if (pENetHost->Init())
        {
            WaitForSingleObject(g_StopEvent, INFINITE);
            pENetHost->Done();
            pENetHost->RegisterRecvProcess(NULL, NULL);
        }
        pENetHost->Release();
    }
}

void ENetClientTest(char* address, int port)
{
    CENetClient* pENetGuest = NULL;

    CLIENT_INFO clientinfo;
    memset(&clientinfo, 0, sizeof(CLIENT_INFO));
    inet_pton(AF_INET, address, &clientinfo.ipaddr);
    clientinfo.port = port;

    pENetGuest = new CENetClient(0, clientinfo, FALSE);
    if (pENetGuest)
    {
        if (pENetGuest->Init())
        {
            while (TRUE)
            {
                HANDLE WaitEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
                HANDLE h[2] = { 0 };
                h[0] = WaitEvent;
                h[1] = g_StopEvent;

                BASE_PACKET_T* Packet = (BASE_PACKET_T*)malloc(DATA_SIZE);
                memset(Packet, 0, DATA_SIZE);
                Packet->Type = 0;
                Packet->Length = DATA_SIZE;

                pENetGuest->SendPacket((PBYTE)Packet, DATA_SIZE, WaitEvent);

                DWORD Ret = WaitForMultipleObjects(2, h, FALSE, INFINITE);

                CloseHandle(WaitEvent);

                if (Ret == WAIT_OBJECT_0)
                {
                    continue;
                }
                else
                {
                    break;
                }
            }
            pENetGuest->Done();
        }

        pENetGuest->Release();
    }
}